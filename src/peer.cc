#include "peer.h"
#include "task.h"
#include "block_pool.h"

namespace Token{
    void PeerSession::OnShutdown(uv_async_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        session->Disconnect();
    }

    void PeerSession::Disconnect(){
        if(pthread_self() == thread_){
            // Inside Session Thread
            uv_read_stop((uv_stream_t*)&socket_);
            uv_stop(socket_.loop);
        } else{
            // Outside Session Thread
            uv_async_send(&shutdown_);
        }
    }

    void* PeerSession::PeerSessionThread(void* data){
        if(!Server::IsRunning() && Server::IsStarting()) Server::WaitForState(Server::kRunning);
        PeerSession* session = (PeerSession*)data;
        NodeAddress address = session->GetAddress();
        LOG(INFO) << "connecting to peer " << address << "....";

        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &session->shutdown_, &OnShutdown);
        uv_tcp_init(loop, &session->socket_);
        uv_tcp_keepalive(&session->socket_, 1, 60);

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
        uv_loop_close(loop);
        pthread_exit(0);
    }

    void PeerSession::OnConnect(uv_connect_t* conn, int status){
        PeerSession* session = (PeerSession*)conn->data;
        if(status != 0){
            LOG(WARNING) << "client accept error: " << uv_strerror(status);
            session->Disconnect();
            return;
        }

        LOG(INFO) << "connected to peer: " << session->GetAddress();
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
        do{
            uint32_t mtype = 0;
            memcpy(&mtype, &buff->base[offset + Message::kTypeOffset], Message::kTypeLength);

            uint64_t msize = 0;
            memcpy(&msize, &buff->base[offset + Message::kSizeOffset], Message::kSizeLength);

            Handle<Message> msg = Message::Decode(static_cast<Message::MessageType>(mtype), msize, (uint8_t*)&buff->base[offset + Message::kDataOffset]);
            LOG(INFO) << "decoded message: " << msg->ToString(); //TODO: handle decode errors
            messages.push_back(msg);

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

    void PeerSession::HandlePingMessage(const Handle<HandleMessageTask>& task){}
    void PeerSession::HandlePongMessage(const Handle<HandleMessageTask>& task){}
    void PeerSession::HandleGetBlocksMessage(const Handle<HandleMessageTask>& task){}

    void PeerSession::HandleVersionMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<VersionMessage> msg = task->GetMessage().CastTo<VersionMessage>();

        std::vector<Handle<Message>> response;
        response.push_back(VerackMessage::NewInstance(Server::GetID()).CastTo<Message>());
        if(BlockChain::GetHead().GetHeight() < msg->GetHeight()){
            response.push_back(GetBlocksMessage::NewInstance().CastTo<Message>());
        }
        session->Send(response);
    }

    void PeerSession::HandleVerackMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<VerackMessage> msg = task->GetMessage().CastTo<VerackMessage>();

        //TODO:
        // - nonce check
        // - state transition
        // - register peer
    }

    void PeerSession::HandlePrepareMessage(const Handle<HandleMessageTask>& task){}

    void PeerSession::HandlePromiseMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<PromiseMessage> msg = task->GetMessage().CastTo<PromiseMessage>();

        if(!BlockMiner::GetProposal()){
            LOG(WARNING) << "no active proposal found";
            return;
        }

        Proposal* proposal = BlockMiner::GetProposal();
        std::string node_id = session->GetID();
        proposal->Vote(node_id);

#ifdef TOKEN_DEBUG
        LOG(INFO) << node_id << " voted for proposal: " << proposal->GetHash();
#endif//TOKEN_DEBUG
    }

    void PeerSession::HandleCommitMessage(const Handle<HandleMessageTask>& task){}

    void PeerSession::HandleAcceptedMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<AcceptedMessage> msg = task->GetMessage().CastTo<AcceptedMessage>();

        if(!BlockMiner::HasProposal()){
            LOG(WARNING) << "no active proposal found";
            return;
        }

        Proposal* proposal = BlockMiner::GetProposal();
        std::string node_id = session->GetID();
#ifdef TOKEN_DEBUG
        LOG(INFO) << node_id << " accepted proposal: " << proposal->GetHash();
#endif//TOKEN_DEBUG
    }

    void PeerSession::HandleRejectedMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<RejectedMessage> msg = task->GetMessage().CastTo<RejectedMessage>();

        if(!BlockMiner::HasProposal()){
            LOG(WARNING) << "no active proposal found";
            return;
        }

        Proposal* proposal = BlockMiner::GetProposal();
        std::string node = msg->GetProposer();
#ifdef TOKEN_DEBUG
        LOG(INFO) << node << " rejected proposal: " << proposal->GetHash();
#endif//TOKEN_DEBUG
    }

    void PeerSession::HandleGetDataMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<GetDataMessage> msg = task->GetMessage().CastTo<GetDataMessage>();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "cannot get items from message";
            return;
        }

        std::vector<Handle<Message>> data;
        for(auto& item : items){
            uint256_t hash = item.GetHash();
            if(item.ItemExists() || BlockPool::HasBlock(hash)){
                LOG(INFO) << "sending " << hash << "....";
                if(item.IsBlock()){
                    Block* block = nullptr;
                    if((block = BlockChain::GetBlockData(hash))){
                        LOG(INFO) << hash << " found in block chain!";
                        data.push_back(BlockMessage::NewInstance(block).CastTo<Message>());
                        continue;
                    }

                    if((block = BlockPool::GetBlock(hash))){
                        LOG(INFO) << hash << " found in block pool!";
                        data.push_back(BlockMessage::NewInstance(block).CastTo<Message>());
                        continue;
                    }
                } else if(item.IsTransaction()){
                    Transaction* tx;
                    if((tx = TransactionPool::GetTransaction(hash))){
                        LOG(INFO) << hash << " found in transaction pool!";
                        data.push_back(TransactionMessage::NewInstance(tx).CastTo<Message>());
                        continue;
                    }
                }
            }
        }
        session->Send(data);
    }

    void PeerSession::HandleBlockMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<BlockMessage> msg = task->GetMessage().CastTo<BlockMessage>();

        Block* block = msg->GetBlock();
        uint256_t hash = block->GetSHA256Hash();
        BlockPool::PutBlock(block);

        LOG(INFO) << "downloaded block: " << block->GetHeader();
        //TODO: session->OnHash(hash);
    }

    void PeerSession::HandleTransactionMessage(const Handle<HandleMessageTask>& task){

    }

    void PeerSession::HandleNotFoundMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<NotFoundMessage> msg = task->GetMessage().CastTo<NotFoundMessage>();

        InventoryItem item = msg->GetItem();
        LOG(WARNING) << "peer " << session->GetID() << " has no record of item: " << item;
    }

    void PeerSession::HandleInventoryMessage(const Handle<HandleMessageTask>& task){
        PeerSession* session = (PeerSession*)task->GetSession();
        Handle<InventoryMessage> msg = task->GetMessage().CastTo<InventoryMessage>();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "couldn't get items from inventory";
            return;
        }

        LOG(INFO) << "received inventory of " << items.size() << " items, downloading...";
        if(!items.empty()) session->Send(GetDataMessage::NewInstance(items));
        //TODO: synchronize blocks
    }

    void PeerSession::HandleTestMessage(const Handle<HandleMessageTask>& task){
        //TODO: implement
    }

    bool PeerSession::Connect(){
        int ret;
        if((ret = pthread_create(&thread_, NULL, &PeerSessionThread, this)) != 0){
            LOG(WARNING) << "couldn't start peer thread: " << strerror(ret);
            return false;
        }
        return true;
    }
}