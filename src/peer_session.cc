#include "peer_session.h"
#include "task.h"
#include "async_task.h"
#include "block_pool.h"
#include "transaction_pool.h"

namespace Token{
    void PeerSession::OnShutdown(uv_async_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        session->Disconnect();
    }

    void PeerSession::OnHeartbeatTick(uv_timer_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        LOG(INFO) << "ping...";
        session->Send(VersionMessage::NewInstance(Server::GetID()).CastTo<Message>());
        uv_timer_start(&session->hb_timeout_, &OnHeartbeatTimeout, Session::kHeartbeatTimeoutMilliseconds, 0);
    }

    void PeerSession::OnHeartbeatTimeout(uv_timer_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        LOG(WARNING) << "peer " << session->GetInfo() << " timed out, disconnecting...";
        session->Disconnect();
    }

    void* PeerSession::SessionThread(void* data){
        Handle<PeerSession> session = (PeerSession*)data;
        uv_loop_t* loop = session->GetLoop();
        NodeAddress address = session->GetAddress();
        LOG(INFO) << "connecting to peer " << address << "....";

        uv_timer_init(loop, &session->hb_timer_);
        uv_timer_init(loop, &session->hb_timeout_);
        uv_async_init(loop, &session->shutdown_, &OnShutdown);
        uv_timer_start(&session->hb_timer_, &OnHeartbeatTick, 90 * 1000, Session::kHeartbeatIntervalMilliseconds);

        struct sockaddr_in addr;
        uv_ip4_addr(address.GetAddress().c_str(), address.GetPort(), &addr);


        uv_connect_t conn;
        conn.data = data;

        int err;
        if((err = uv_tcp_connect(&conn, session->GetHandle(), (const struct sockaddr*)&addr, &PeerSession::OnConnect)) != 0){
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

        Server::RegisterPeer(session);
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

        Handle<Buffer> rbuff = session->GetReadBuffer();
        do{
            uint32_t mtype = rbuff->GetInt();
            intptr_t msize = rbuff->GetLong();

            switch(mtype) {
#define DEFINE_DECODE(Name) \
                case Message::MessageType::k##Name##MessageType:{ \
                    Handle<Message> msg = Name##Message::NewInstance(rbuff).CastTo<Message>(); \
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
        rbuff->Reset();
    }

    void PeerSession::HandleGetBlocksMessage(const Handle<HandleMessageTask>& task){}
    void PeerSession::HandleGetUnclaimedTransactionsMessage(const Handle<HandleMessageTask>& task){}

    void PeerSession::HandleVersionMessage(const Handle<HandleMessageTask>& task){
        Handle<PeerSession> session = task->GetSession().CastTo<PeerSession>();
        Handle<VersionMessage> msg = task->GetMessage().CastTo<VersionMessage>();

        session->Send(VerackMessage::NewInstance(ClientType::kNode, Server::GetID(), Server::GetCallbackAddress()).CastTo<Message>());
    }

    void PeerSession::HandleVerackMessage(const Handle<HandleMessageTask>& task){
        Handle<PeerSession> session = task->GetSession().CastTo<PeerSession>();
        Handle<VerackMessage> msg = task->GetMessage().CastTo<VerackMessage>();

        LOG(INFO) << "remote timestamp: " << GetTimestampFormattedReadable(msg->GetTimestamp());
        LOG(INFO) << "remote <HEAD>: " << msg->GetHead();

        //TODO: session->SetHead(msg->GetHead());
        if(session->IsConnecting()){
            session->SetInfo(Peer(msg->GetID(), msg->GetCallbackAddress()));
            LOG(INFO) << "connected to peer: " << session->GetInfo();
            session->SetState(Session::kConnected);

            BlockHeader local_head = BlockChain::GetHead()->GetHeader();
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
        TODO:
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
        TODO:
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
        TODO:
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
        Handle<PeerSession> session = task->GetSession().CastTo<PeerSession>();
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
        Handle<PeerSession> session = task->GetSession().CastTo<PeerSession>();
        Handle<BlockMessage> msg = task->GetMessage().CastTo<BlockMessage>();

        Handle<Block> blk = msg->GetBlock();
        BlockPool::PutBlock(blk);
        LOG(INFO) << "received block: " << blk;
    }

    void PeerSession::HandleTransactionMessage(const Handle<HandleMessageTask>& task){

    }

    void PeerSession::HandleUnclaimedTransactionMessage(const Handle<HandleMessageTask>& task){

    }

    void PeerSession::HandleNotFoundMessage(const Handle<HandleMessageTask>& task){
        Handle<PeerSession> session = task->GetSession().CastTo<PeerSession>();
        Handle<NotFoundMessage> msg = task->GetMessage().CastTo<NotFoundMessage>();
        LOG(WARNING) << "(" << session->GetInfo() << "): " << msg->GetMessage();
    }

    void PeerSession::HandleInventoryMessage(const Handle<HandleMessageTask>& task){
        Handle<PeerSession> session = task->GetSession().CastTo<PeerSession>();
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

    void PeerSession::HandleGetPeersMessage(const Handle<HandleMessageTask>& task){
        //TODO: implement PeerSession::HandleGetPeersMessage(const Handle<HandleMessageTask>&);
    }

    void PeerSession::HandlePeerListMessage(const Handle<HandleMessageTask>& task){
        //TODO: implement PeerSession::HandlePeerListMessage(const Handle<HandleMessageTask>&);
    }
}