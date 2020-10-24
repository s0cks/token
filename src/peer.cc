#include "peer.h"
#include "task.h"
#include "async_task.h"
#include "block_pool.h"
#include "byte_buffer.h"

namespace Token{
    void PeerSession::OnShutdown(uv_async_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        session->Disconnect();
    }

    uv_loop_t* PeerSession::GetLoop() const{
        return socket_.loop;// is this safe?
    }

    void PeerSession::Disconnect(){
        if(pthread_self() == thread_){
            // Inside Session OSThreadBase
            uv_read_stop((uv_stream_t*)&socket_);
            uv_stop(socket_.loop);
            uv_loop_close(socket_.loop);
            Server::UnregisterPeer(this);
            SetState(State::kDisconnected);
        } else{
            // Outside Session OSThreadBase
            uv_async_send(&shutdown_);
        }
    }

    void PeerSession::OnHeartbeatTick(uv_timer_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        LOG(INFO) << "ping...";
        session->Send(VersionMessage::NewInstance(Server::GetID()).CastTo<Message>());
        uv_timer_start(&session->hb_timeout_, &OnHeartbeatTimeout, Session::kHeartbeatTimeoutMilliseconds, 0);
    }

    void PeerSession::OnHeartbeatTimeout(uv_timer_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        LOG(WARNING) << "peer " << session->GetID() << " timed out, disconnecting...";
        session->Disconnect();
    }

    void* PeerSession::PeerSessionThread(void* data){
        Server::WaitForState(Server::kRunning);

        PeerSession* session = (PeerSession*)data;
        NodeAddress address = session->GetAddress();
        LOG(INFO) << "connecting to peer " << address << "....";

        uv_loop_t* loop = uv_loop_new();
        uv_timer_init(loop, &session->hb_timer_);
        uv_timer_init(loop, &session->hb_timeout_);
        uv_async_init(loop, &session->shutdown_, &OnShutdown);
        uv_tcp_init(loop, &session->socket_);
        uv_tcp_keepalive(&session->socket_, 1, 60);

        uv_timer_start(&session->hb_timer_, &OnHeartbeatTick, 90 * 1000, Session::kHeartbeatIntervalMilliseconds);

        struct sockaddr_in addr;
        uv_ip4_addr(address.GetAddress().c_str(), address.GetPort(), &addr);

        int err;
        if((err = uv_tcp_connect(&session->conn_, &session->socket_, (const struct sockaddr*)&addr, &OnConnect)) != 0){
            LOG(WARNING) << "couldn't connect to peer " << address << ": " << uv_strerror(err);
            session->Disconnect();
            goto cleanup;
        }

        uv_run(loop, UV_RUN_DEFAULT);
    cleanup:
        LOG(INFO) << "disconnected from peer: " << address;
        pthread_exit(0);
    }

    void PeerSession::OnConnect(uv_connect_t* conn, int status){
        PeerSession* session = (PeerSession*)conn->data;
        if(status != 0){
            LOG(WARNING) << "client accept error: " << uv_strerror(status);
            session->Disconnect();
            return;
        }

        session->SetState(Session::kConnecting);
        session->Send(VersionMessage::NewInstance(Server::GetID()));
        if((status = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
            LOG(WARNING) << "client read error: " << uv_strerror(status);
            session->Disconnect();
            return;
        }
    }

    void PeerSession::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        PeerSession* session = (PeerSession*)stream->data;

        if(nread == UV_EOF){
            LOG(ERROR) << "client disconnected!";
            return;
        } else if(nread < 0){
            LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
            return;
        } else if(nread == 0){
            LOG(WARNING) << "zero message size received";
            return;
        } else if(nread >= 4096){
            LOG(ERROR) << "too large of a buffer";
            return;
        }

        uint32_t offset = 0;
        std::vector<Handle<Message>> messages;
        ByteBuffer bytes((uint8_t*)buff->base, buff->len);
        do{
            uint32_t mtype = bytes.GetInt();
            intptr_t msize = bytes.GetLong();

            switch(mtype) {
#define DEFINE_DECODE(Name) \
                case Message::MessageType::k##Name##MessageType:{ \
                    Handle<Message> msg = Name##Message::NewInstance(&bytes).CastTo<Message>(); \
                    LOG(INFO) << "decoded: " << msg; \
                    messages.push_back(msg); \
                    break; \
                }
                FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
                case Message::MessageType::kUnknownMessageType:
                default:
                    LOG(ERROR) << "unknown message type " << mtype << " of size " << msize;
                    break;
            }

            offset += (msize + Message::kHeaderSize);
        } while((offset + Message::kHeaderSize) < nread);

        for(size_t idx = 0; idx < messages.size(); idx++){
            Handle<Message> msg = messages[idx];
            Handle<HandleMessageTask> task = HandleMessageTask::NewInstance(session, msg);
            switch(msg->GetMessageType()){
#define DEFINE_HANDLER_CASE(Name) \
            case Message::k##Name##MessageType: \
                session->Handle##Name##Message(task); \
                break;
                FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE);
#undef DEFINE_HANDLER_CASE
                case Message::kUnknownMessageType:
                default: //TODO: handle properly
                    break;
            }
        }
    }

    void PeerSession::HandleGetBlocksMessage(const Handle<HandleMessageTask>& task){}
    void PeerSession::HandleGetUnclaimedTransactionsMessage(const Handle<HandleMessageTask>& task){}

    void PeerSession::HandleVersionMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<VersionMessage> msg = task->GetMessage().CastTo<VersionMessage>();

        NodeAddress callback("127.0.0.1", FLAGS_port);//TODO: dynamically lookup callback address
        session->Send(VerackMessage::NewInstance(ClientType::kNode, Server::GetID(), callback).CastTo<Message>());
    }

    void PeerSession::HandleVerackMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<VerackMessage> msg = task->GetMessage().CastTo<VerackMessage>();

        LOG(INFO) << "remote timestamp: " << GetTimestampFormattedReadable(msg->GetTimestamp());
        LOG(INFO) << "remote <HEAD>: " << msg->GetHead();

        session->SetHead(msg->GetHead());
        if(IsConnecting()){
            LOG(INFO) << "connected to peer: " << session->GetID() << "[" << session->GetAddress() << "]";
            Server::RegisterPeer(session);
            session->SetID(msg->GetID());
            session->SetState(Session::kConnected);

            BlockHeader local_head = BlockChain::GetHead();
            BlockHeader remote_head = msg->GetHead();
            if(local_head == remote_head){
                LOG(INFO) << "skipping remote <HEAD> := " << remote_head;
            } else if(local_head < remote_head){
                Handle<SynchronizeBlockChainTask> sync_task = SynchronizeBlockChainTask::NewInstance(session->GetLoop(), session, remote_head);
                sync_task->Submit();

                session->Send(GetBlocksMessage::NewInstance().CastTo<Message>());
            }
        }

        //TODO:
        // - nonce check
        // - state transition
    }

    void PeerSession::HandlePrepareMessage(const Handle<HandleMessageTask>& task){}

    void PeerSession::HandlePromiseMessage(const Handle<HandleMessageTask>& task){
        Handle<PromiseMessage> msg = task->GetMessage().CastTo<PromiseMessage>();

        /*
        if(!ProposerThread::HasProposal()){
            LOG(WARNING) << "no active proposal found";
            return;
        }

        Handle<Proposal> proposal = ProposerThread::GetProposal();
        std::string node_id = session->GetID();
        proposal->AcceptProposal(node_id);
#ifdef TOKEN_DEBUG
        LOG(INFO) << node_id << " voted for proposal: " << proposal->GetHash();
#endif//TOKEN_DEBUG
        */
    }

    void PeerSession::HandleCommitMessage(const Handle<HandleMessageTask>& task){}

    void PeerSession::HandleAcceptedMessage(const Handle<HandleMessageTask>& task){
        Handle<AcceptedMessage> msg = task->GetMessage().CastTo<AcceptedMessage>();

        /*
        if(!ProposerThread::HasProposal()){
            LOG(WARNING) << "no active proposal found";
            return;
        }

        Proposal* proposal = ProposerThread::GetProposal();
        std::string node_id = session->GetID();
        proposal->AcceptProposal(node_id);
#ifdef TOKEN_DEBUG
        LOG(INFO) << node_id << " accepted proposal: " << proposal->GetHash();
#endif//TOKEN_DEBUG
         */
    }

    void PeerSession::HandleRejectedMessage(const Handle<HandleMessageTask>& task){
        Handle<RejectedMessage> msg = task->GetMessage().CastTo<RejectedMessage>();

        /*
        if(!ProposerThread::HasProposal()){
            LOG(WARNING) << "no active proposal found";
            return;
        }

        Proposal* proposal = ProposerThread::GetProposal();
        std::string node = msg->GetProposer();
        proposal->RejectProposal(node);
#ifdef TOKEN_DEBUG
        LOG(INFO) << node << " rejected proposal: " << proposal->GetHash();
#endif//TOKEN_DEBUG
    */
    }

    void PeerSession::HandleGetDataMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<GetDataMessage> msg = task->GetMessage().CastTo<GetDataMessage>();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "cannot get items from message";
            return;
        }

        LOG(INFO) << "getting " << items.size() << " items....";
        std::vector<Handle<Message>> response;
        for(auto& item : items){
            Hash hash = item.GetHash();
            LOG(INFO) << "resolving item : " << hash;
            if(item.IsBlock()){
                if(BlockChain::HasBlock(hash)){
                    LOG(INFO) << "item " << hash << " found in block chain";
                    response.push_back(BlockMessage::NewInstance(BlockChain::GetBlock(hash)).CastTo<Message>());
                    continue;
                }

                if(BlockPool::HasBlock(hash)){
                    LOG(INFO) << "item " << hash << " found in block pool";
                    response.push_back(BlockMessage::NewInstance(BlockPool::GetBlock(hash)).CastTo<Message>());
                    continue;
                }

                LOG(WARNING) << "couldn't find requested block: " << hash;
                response.push_back(NotFoundMessage::NewInstance().CastTo<Message>());
            } else if(item.IsTransaction()){
                if(TransactionPool::HasTransaction(hash)){
                    LOG(INFO) << "item " << hash << " found in transaction pool";
                    response.push_back(TransactionMessage::NewInstance(TransactionPool::GetTransaction(hash)).CastTo<Message>());
                    continue;
                }

                LOG(WARNING) << "couldn't find requested transaction: " << hash;
                response.push_back(NotFoundMessage::NewInstance().CastTo<Message>());
            }
        }
        session->Send(response);
    }

    void PeerSession::HandleBlockMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<BlockMessage> msg = task->GetMessage().CastTo<BlockMessage>();

        Block* block = msg->GetBlock();
        Hash hash = block->GetHash();
        BlockPool::PutBlock(block);

        LOG(INFO) << "downloaded block: " << block->GetHeader();
        session->OnItemReceived(InventoryItem(InventoryItem::kBlock, hash));
    }

    void PeerSession::HandleTransactionMessage(const Handle<HandleMessageTask>& task){

    }

    void PeerSession::HandleUnclaimedTransactionMessage(const Handle<HandleMessageTask>& task){

    }

    void PeerSession::HandleNotFoundMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<NotFoundMessage> msg = task->GetMessage().CastTo<NotFoundMessage>();
        LOG(WARNING) << "(" << session->GetID() << "): " << msg->GetMessage();
    }

    void PeerSession::HandleInventoryMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<InventoryMessage> msg = task->GetMessage().CastTo<InventoryMessage>();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "couldn't get items from inventory message";
            return;
        }

        std::vector<InventoryItem> needed;
        for(auto& item : items){
            if(!item.ItemExists()) needed.push_back(item);
        }

        LOG(INFO) << "downloading " << needed.size() << "/" << items.size() << " items from inventory....";
        session->Send(GetDataMessage::NewInstance(items));
    }

    bool PeerSession::Connect(){
        int ret;
        if((ret = pthread_create(&thread_, NULL, &PeerSessionThread, this)) != 0){
            LOG(WARNING) << "couldn't start peer thread: " << strerror(ret);
            return false;
        }
        return true;
    }

    void PeerSession::HandleGetPeersMessage(const Handle<HandleMessageTask>& task){
        //TODO: implement PeerSession::HandleGetPeersMessage(const Handle<HandleMessageTask>&);
    }

    void PeerSession::HandlePeerListMessage(const Handle<HandleMessageTask>& task){
        //TODO: implement PeerSession::HandlePeerListMessage(const Handle<HandleMessageTask>&);
    }
}