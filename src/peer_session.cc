#include "peer_session.h"
#include "task.h"
#include "server.h"
#include "async_task.h"
#include "configuration.h"
#include "block_discovery.h"

namespace Token{
    void PeerSession::OnShutdown(uv_async_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        session->Disconnect();
    }

    void PeerSession::OnPrepare(uv_async_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        if(!BlockDiscoveryThread::HasProposal()){
            LOG(WARNING) << "there is no active proposal.";
            return;
        }

        Proposal* proposal = BlockDiscoveryThread::GetProposal();
        PrepareMessage msg(proposal);
        session->Send(&msg);
    }

    void PeerSession::OnPromise(uv_async_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        if(!BlockDiscoveryThread::HasProposal()){
            LOG(WARNING) << "there is no active proposal.";
            return;
        }

        Proposal* proposal = BlockDiscoveryThread::GetProposal();
        if(!proposal->IsProposal()){
            LOG(WARNING) << "cannot send another promise to the peer.";
            return;
        }

        PromiseMessage msg(proposal);
        session->Send(&msg);
    }

    void PeerSession::OnCommit(uv_async_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        if(!BlockDiscoveryThread::HasProposal()){
            LOG(WARNING) << "there is no active proposal.";
            return;
        }

        Proposal* proposal = BlockDiscoveryThread::GetProposal();
        if(!proposal->IsCommit()){
            LOG(WARNING) << "cannot send another commit to the peer.";
            return;
        }

        CommitMessage msg(proposal);
        session->Send(&msg);
    }

    void PeerSession::OnAccepted(uv_async_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        if(!BlockDiscoveryThread::HasProposal()){
            LOG(WARNING) << "there is no active proposal.";
            return;
        }

        Proposal* proposal = BlockDiscoveryThread::GetProposal();
        if(!proposal->IsVoting() && !proposal->IsCommit()){
            LOG(WARNING) << "cannot accept proposal #" << proposal->GetHeight() << " (" << proposal->GetPhase() << " [" << proposal->GetResult() << "])";
            return;
        }

        AcceptedMessage msg(proposal);
        session->Send(&msg);
    }

    void PeerSession::OnRejected(uv_async_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        if(!BlockDiscoveryThread::HasProposal()){
            LOG(WARNING) << "there is no active proposal.";
            return;
        }

        Proposal* proposal = BlockDiscoveryThread::GetProposal();
        if(!proposal->IsVoting() && !proposal->IsCommit()){
            LOG(WARNING) << "cannot reject proposal #" << proposal->GetHeight() << " (" << proposal->GetPhase() << " [" << proposal->GetResult() << "])";
            return;
        }

        RejectedMessage msg(proposal);
        session->Send(&msg);
    }

    void PeerSession::OnHeartbeatTick(uv_timer_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        LOG(INFO) << "ping...";
        session->Send(VersionMessage::NewInstance(Server::GetID()));
        uv_timer_start(&session->hb_timeout_, &OnHeartbeatTimeout, Session::kHeartbeatTimeoutMilliseconds, 0);
    }

    void PeerSession::OnHeartbeatTimeout(uv_timer_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        LOG(WARNING) << "peer " << session->GetInfo() << " timed out, disconnecting...";
        session->Disconnect();
    }

    void* PeerSession::SessionThread(void* data){
        PeerSession* session = (PeerSession*)data;
        uv_loop_t* loop = session->GetLoop();
        NodeAddress address = session->GetAddress();
        LOG(INFO) << "connecting to peer " << address << "....";

        uv_timer_init(loop, &session->hb_timer_);
        uv_timer_init(loop, &session->hb_timeout_);
        uv_async_init(loop, &session->shutdown_, &OnShutdown);

        uv_async_init(loop, &session->prepare_, &OnPrepare);
        uv_async_init(loop, &session->promise_, &OnPromise);
        uv_async_init(loop, &session->commit_, &OnCommit);
        uv_async_init(loop, &session->accepted_, &OnAccepted);
        uv_async_init(loop, &session->rejected_, &OnRejected);
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

        Block* head = BlockChain::GetHead();
        session->Send(VersionMessage::NewInstance(ClientType::kNode, Server::GetID(), head->GetHeader()));
        delete head;

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
        std::vector<Message*> messages;

        Buffer* rbuff = session->GetReadBuffer();
        do{
            uint32_t mtype = rbuff->GetInt();
            intptr_t msize = rbuff->GetLong();

            switch(mtype) {
#define DEFINE_DECODE(Name) \
                case Message::MessageType::k##Name##MessageType:{ \
                    Message* msg = Name##Message::NewInstance(rbuff); \
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
            Message* msg = messages[idx];
            HandleMessageTask* task = HandleMessageTask::NewInstance(session, msg);
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

    void PeerSession::HandleGetBlocksMessage(HandleMessageTask* task){}
    void PeerSession::HandleGetUnclaimedTransactionsMessage(HandleMessageTask* task){}

    void PeerSession::HandleVersionMessage(HandleMessageTask* task){
        PeerSession* session = task->GetSession<PeerSession>();
        Block* head= BlockChain::GetHead();
        VerackMessage msg(ClientType::kNode, Server::GetID(), head->GetHeader());
        session->Send(&msg);
        delete head;
    }

    void PeerSession::HandleVerackMessage(HandleMessageTask* task){
        PeerSession* session = task->GetSession<PeerSession>();
        VerackMessage* msg = task->GetMessage<VerackMessage>();

        LOG(INFO) << "remote timestamp: " << GetTimestampFormattedReadable(msg->GetTimestamp());
        LOG(INFO) << "remote <HEAD>: " << msg->GetHead();

        //TODO: session->SetHead(msg->GetHead());
        if(session->IsConnecting()){
            Server::RegisterPeer(session);
            session->SetInfo(Peer(msg->GetID(), msg->GetCallbackAddress()));
            LOG(INFO) << "connected to peer: " << session->GetInfo();
            session->SetState(Session::kConnected);

            BlockHeader local_head = BlockChain::GetHead()->GetHeader();
            BlockHeader remote_head = msg->GetHead();
            if(local_head == remote_head){
                LOG(INFO) << "skipping remote <HEAD> := " << remote_head;
            } else if(local_head < remote_head){
                SynchronizeBlockChainTask* sync_task = SynchronizeBlockChainTask::NewInstance(session->GetLoop(), session, remote_head);
                sync_task->Submit();
                session->Send(GetBlocksMessage::NewInstance());
            }
        }

        //TODO:
        // - nonce check
        // - state transition
    }

    void PeerSession::HandlePrepareMessage(HandleMessageTask* task){}
    void PeerSession::HandlePromiseMessage(HandleMessageTask* task){}
    void PeerSession::HandleCommitMessage(HandleMessageTask* task){}
    void PeerSession::HandleAcceptedMessage(HandleMessageTask* task){}
    void PeerSession::HandleRejectedMessage(HandleMessageTask* task){}

    void PeerSession::HandleGetDataMessage(HandleMessageTask* task){
        PeerSession* session = (PeerSession*)task->GetSession();
        GetDataMessage* msg = (GetDataMessage*)task->GetMessage();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "cannot get items from message";
            return;
        }

        LOG(INFO) << "getting " << items.size() << " items....";
        std::vector<Message*> response;
        for(auto& item : items){
            Hash hash = item.GetHash();
            LOG(INFO) << "resolving item : " << hash;
            if(item.IsBlock()){
                Block* block = nullptr;
                if(BlockChain::HasBlock(hash)){
                    LOG(INFO) << "item " << hash << " found in block chain";
                    block = BlockChain::GetBlock(hash);
                } else if(BlockPool::HasBlock(hash)){
                    LOG(INFO) << "item " << hash << " found in block pool";
                    block = BlockPool::GetBlock(hash);
                } else{
                    LOG(WARNING) << "cannot find block: " << hash;
                    response.push_back(NotFoundMessage::NewInstance());
                    break;
                }
                response.push_back(new BlockMessage((*block)));
                delete block;
            } else if(item.IsTransaction()){
                if(!TransactionPool::HasTransaction(hash)){
                    LOG(WARNING) << "cannot find transaction: " << hash;
                    response.push_back(NotFoundMessage::NewInstance());
                    break;
                }

                Transaction* tx = TransactionPool::GetTransaction(hash);
                response.push_back(new TransactionMessage((*tx)));
                delete tx;
            }
        }
        session->Send(response);
    }

    void PeerSession::HandleBlockMessage(HandleMessageTask* task){
        BlockMessage* msg = (BlockMessage*)task->GetMessage();
        Block& blk = msg->GetBlock();
        Hash bhash = blk.GetHash();
        BlockPool::PutBlock(bhash, &blk);
        LOG(INFO) << "received block: " << bhash;
    }

    void PeerSession::HandleTransactionMessage(HandleMessageTask* task){

    }

    void PeerSession::HandleUnclaimedTransactionMessage(HandleMessageTask* task){

    }

    void PeerSession::HandleNotFoundMessage(HandleMessageTask* task){
        PeerSession* session = (PeerSession*)task->GetSession();
        NotFoundMessage* msg = (NotFoundMessage*)task->GetMessage();
        LOG(WARNING) << "(" << session->GetInfo() << "): " << msg->GetMessage();
    }

    void PeerSession::HandleInventoryMessage(HandleMessageTask* task){
        PeerSession* session = (PeerSession*)task->GetSession();
        InventoryMessage* msg = (InventoryMessage*)task->GetMessage();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "couldn't get items from inventory message";
            return;
        }

        std::vector<InventoryItem> needed;
        for(auto& item : items){
            if(!item.ItemExists())
                needed.push_back(item);
        }

        LOG(INFO) << "downloading " << needed.size() << "/" << items.size() << " items from inventory....";
        session->Send(GetDataMessage::NewInstance(items));
    }

    void PeerSession::HandleGetPeersMessage(HandleMessageTask* task){
        //TODO: implement PeerSession::HandleGetPeersMessage(HandleMessageTask*);
    }

    void PeerSession::HandlePeerListMessage(HandleMessageTask* task){
        //TODO: implement PeerSession::HandlePeerListMessage(HandleMessageTask*);
    }
}