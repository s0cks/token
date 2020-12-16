#include <uuid/uuid.h>
#include <algorithm>
#include <random>
#include <condition_variable>
#include "server.h"
#include "message.h"
#include "task/task.h"
#include "configuration.h"
#include "proposal.h"
#include "unclaimed_transaction.h"
#include "block_discovery.h"

#include "task/handle_message_task.h"
#include "peer/peer_session_manager.h"

namespace Token{
    static uv_tcp_t handle_;
    static uv_async_t shutdown_;

    static std::mutex mutex_;
    static std::condition_variable cond_;
    static Server::State state_ = Server::State::kStopped;
    static Server::Status status_ = Server::Status::kOk;
    static UUID node_id_;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    uv_tcp_t* Server::GetHandle(){
        return &handle_;
    }

    UUID Server::GetID(){
        LOCK_GUARD;
        return node_id_;
    }

    /*
    bool Server::Shutdown(){
        if(!IsRunning()) return false;
        uv_async_send(&aterm_);

        int ret;
        if((ret = pthread_join(thread_, NULL)) != 0){
            std::stringstream ss;
            ss << "Couldn't join server thread: " << strerror(ret);
            CrashReport::GenerateAndExit(ss);
        }

        SetState(State::kStopped);
        return true;
    }
    */

    void Server::WaitForState(Server::State state){
        LOCK;
        while(state_ != state) WAIT;
    }

    void Server::SetState(Server::State state){
        LOCK;
        state_ = state;
        SIGNAL_ALL;
    }

    Server::State Server::GetState() {
        LOCK_GUARD;
        return state_;
    }

    void Server::SetStatus(Server::Status status){
        LOCK;
        status_ = status;
        SIGNAL_ALL;
    }

    Server::Status Server::GetStatus(){
        LOCK_GUARD;
        return status_;
    }

    std::string Server::GetStatusMessage(){
        std::stringstream ss;
        LOCK_GUARD;
        ss << "[";
        switch(state_){
#define DEFINE_STATE_MESSAGE(Name) \
            case Server::k##Name: \
                ss << #Name; \
                break;
            FOR_EACH_SERVER_STATE(DEFINE_STATE_MESSAGE)
#undef DEFINE_STATE_MESSAGE
        }
        ss << "] ";

        switch(status_){
#define DEFINE_STATUS_MESSAGE(Name) \
            case Server::k##Name:{ \
                ss << #Name; \
                break; \
            }
            FOR_EACH_SERVER_STATUS(DEFINE_STATUS_MESSAGE)
#undef DEFINE_STATUS_MESSAGE
        }
        return ss.str();
    }

    void Server::HandleTerminateCallback(uv_async_t* handle){
        uv_stop(handle->loop);
    }

    bool Server::Initialize(){
        if(!IsStopped()){
            LOG(WARNING) << "the server is already running.";
            return false;
        }

        LOG(INFO) << "initializing the server....";
        std::set<NodeAddress> peers;
        if(!BlockChainConfiguration::GetPeerList(peers)){
            LOG(WARNING) << "couldn't get the list of peers.";
            return false;
        }

        LOG(INFO) << "connecting to peers....";
        //TODO: ConnectToPeers(peers);
        return Thread::Start("ServerThread", &HandleThread, 0) == 0;
    }

    bool Server::Shutdown(){
        //TODO: implement Server::Shutdown()
        LOG(WARNING) << "Server::Shutdown() not implemented yet.";
        if(pthread_self() == uv_thread_self()){
            // Inside OSThreadBase
        } else{
            // Outside OSThreadBase
            uv_async_send(&shutdown_);
        }
        return false;
    }

    void Server::HandleThread(uword parameter){
        LOG(INFO) << "starting server...";
        SetState(State::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &shutdown_, &HandleTerminateCallback);

        struct sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", FLAGS_port, &addr);
        uv_tcp_init(loop, GetHandle());
        uv_tcp_keepalive(GetHandle(), 1, 60);

        int err;
        if((err = uv_tcp_bind(GetHandle(), (const struct sockaddr*)&addr, 0)) != 0){
            LOG(ERROR) << "couldn't bind the server: " << uv_strerror(err);
            goto exit;
        }

        if((err = uv_listen((uv_stream_t*)GetHandle(), 100, &OnNewConnection)) != 0){
            LOG(ERROR) << "server couldn't listen: " << uv_strerror(err);
            goto exit;
        }



        LOG(INFO) << "server " << GetID() << " listening @" << FLAGS_port;
        SetState(State::kRunning);
        uv_run(loop, UV_RUN_DEFAULT);
    exit:
        uv_loop_close(loop);
        pthread_exit(0);
    }

    void Server::OnNewConnection(uv_stream_t* stream, int status){
        if(status != 0){
            LOG(ERROR) << "connection error: " << uv_strerror(status);
            return;
        }

        ServerSession* session = ServerSession::NewInstance(stream->loop);
        session->SetState(Session::kConnecting);
        LOG(INFO) << "client is connecting...";

        int err;
        if((err = uv_accept(stream, (uv_stream_t*)session->GetStream())) != 0){
            LOG(ERROR) << "client accept error: " << uv_strerror(err);
            return;
        }

        LOG(INFO) << "client connected";
        if((err = uv_read_start(session->GetStream(), &ServerSession::AllocBuffer, &OnMessageReceived)) != 0){
            LOG(ERROR) << "client read error: " << uv_strerror(err);
            return;
        }
    }

    void Server::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        ServerSession* session = (ServerSession*)stream->data;
        if(nread == UV_EOF){
            LOG(ERROR) << "client disconnected!";
            return;
        } else if(nread < 0){
            LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
            return;
        } else if(nread == 0){
            LOG(WARNING) << "zero message size received";
            return;
        } else if(nread > Session::kBufferSize){
            LOG(ERROR) << "too large of a buffer: " << nread;
            return;
        }

        Buffer* rbuff = session->GetReadBuffer();

        intptr_t offset = 0;
        std::vector<Message*> messages;
        do{
            int32_t mtype = rbuff->GetInt();
            int64_t  msize = rbuff->GetLong();

            switch(mtype) {
#define DEFINE_DECODE(Name) \
                case Message::MessageType::k##Name##MessageType:{ \
                    Message* msg = Name##Message::NewInstance(rbuff); \
                    LOG(INFO) << "received message: " << msg->ToString(); \
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

    void ServerSession::HandleNotFoundMessage(HandleMessageTask* task){
        //TODO: implement HandleNotFoundMessage
        LOG(WARNING) << "not implemented";
        return;
    }

    void ServerSession::HandleVersionMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        //TODO:
        // - state check
        // - version check
        // - echo nonce
        session->Send(VersionMessage::NewInstance(Server::GetID()));
    }

    //TODO:
    // - verify nonce
    void ServerSession::HandleVerackMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        VerackMessage* msg = task->GetMessage<VerackMessage>();

        BlockPtr head = BlockChain::GetHead();
        VerackMessage verack(ClientType::kNode, Server::GetID(), head->GetHeader());
        session->Send(&verack);

        UUID id = msg->GetID();
        if(session->IsConnecting()){
            session->SetID(id);
            session->SetState(Session::State::kConnected);
            if(msg->IsNode()){
                NodeAddress paddr = msg->GetCallbackAddress();
                LOG(INFO) << "peer connected from: " << paddr;
                PeerSessionManager::ConnectTo(paddr);
            }
        }
    }

    void ServerSession::HandleGetDataMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        GetDataMessage* msg = task->GetMessage<GetDataMessage>();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "cannot get items from message";
            return;
        }

        std::vector<Message*> response;
        for(auto& item : items){
            Hash hash = item.GetHash();
            LOG(INFO) << "searching for: " << hash;
            if(item.ItemExists()){
                if(item.IsBlock()){
                    BlockPtr block;
                    if(BlockChain::HasBlock(hash)){
                        block = BlockChain::GetBlock(hash);
                    } else if(BlockPool::HasBlock(hash)){
                        block = BlockPool::GetBlock(hash);
                    } else{
                        LOG(WARNING) << "cannot find block: " << hash;
                        response.push_back(NotFoundMessage::NewInstance());
                        break;
                    }
                    response.push_back(new BlockMessage(block));
                } else if(item.IsTransaction()){
                    if(!TransactionPool::HasTransaction(hash)){
                        LOG(WARNING) << "cannot find transaction: " << hash;
                        response.push_back(NotFoundMessage::NewInstance());
                        break;
                    }

                    TransactionPtr tx = TransactionPool::GetTransaction(hash);
                    response.push_back(new TransactionMessage(tx));//TODO: fix
                } else if(item.IsUnclaimedTransaction()){
                    if(!UnclaimedTransactionPool::HasUnclaimedTransaction(hash)){
                        LOG(WARNING) << "couldn't find unclaimed transaction: " << hash;
                        response.push_back(NotFoundMessage::NewInstance());
                        break;
                    }

                    UnclaimedTransactionPtr utxo = UnclaimedTransactionPool::GetUnclaimedTransaction(hash);
                    response.push_back(new UnclaimedTransactionMessage(utxo));
                }
            } else{
                session->Send(NotFoundMessage::NewInstance());
                return;
            }
        }

        session->Send(response);
    }

    void ServerSession::HandlePrepareMessage(HandleMessageTask* task){
        PrepareMessage* msg = task->GetMessage<PrepareMessage>();
        ProposalPtr proposal = msg->GetProposal();
        if(!proposal){
            LOG(ERROR) << "cannot get proposal from BlockDiscoveryThread.";
            return;
        }

        proposal->SetPhase(Proposal::kProposalPhase);
    }

    void ServerSession::HandlePromiseMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        PromiseMessage* msg = task->GetMessage<PromiseMessage>();
        ProposalPtr proposal = msg->GetProposal();
        if(proposal->GetProposer() == Server::GetID()){
            proposal->AcceptProposal(session->GetID().ToString());//TODO: remove ToString()
        } else{
            proposal->SetPhase(Proposal::kVotingPhase);
        }
    }

    void ServerSession::HandleCommitMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        CommitMessage* msg = task->GetMessage<CommitMessage>();
        ProposalPtr proposal = msg->GetProposal();
        if(proposal->GetProposer() == Server::GetID()){
            proposal->AcceptProposal(session->GetID().ToString());//TODO: remove ToString()
        } else{
            proposal->SetPhase(Proposal::kCommitPhase);
        }
    }

    void ServerSession::HandleAcceptedMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        AcceptedMessage* msg = task->GetMessage<AcceptedMessage>();
        ProposalPtr proposal = msg->GetProposal();
        proposal->AcceptProposal(session->GetID().ToString()); //TODO: remove ToString()
    }

    void ServerSession::HandleRejectedMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        AcceptedMessage* msg = task->GetMessage<AcceptedMessage>();
        ProposalPtr proposal = msg->GetProposal();
        proposal->RejectProposal(session->GetID().ToString()); //TODO: remove ToString()
    }

    void ServerSession::HandleBlockMessage(HandleMessageTask* task){
        BlockMessage* msg = task->GetMessage<BlockMessage>();
        BlockPtr blk = msg->GetValue();
        Hash hash = blk->GetHash();
        if(!BlockPool::HasBlock(hash)){
            BlockPool::PutBlock(hash, blk);
            //TODO: Server::Broadcast(InventoryMessage::NewInstance(blk));
        }
    }

    void ServerSession::HandleTransactionMessage(HandleMessageTask* task){
        TransactionMessage* msg = task->GetMessage<TransactionMessage>();
        TransactionPtr tx = msg->GetValue();
        Hash hash = tx->GetHash();

        LOG(INFO) << "received transaction: " << hash;
        if(!TransactionPool::HasTransaction(hash))
            TransactionPool::PutTransaction(hash, tx);
    }

    void ServerSession::HandleUnclaimedTransactionMessage(HandleMessageTask* task){

    }

    void ServerSession::HandleInventoryMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        InventoryMessage* msg = task->GetMessage<InventoryMessage>();

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
        if(!needed.empty()) session->Send(GetDataMessage::NewInstance(needed));
    }

    class UserUnclaimedTransactionCollector : public UnclaimedTransactionPoolVisitor{
    private:
        std::vector<InventoryItem>& items_;
        User user_;
    public:
        UserUnclaimedTransactionCollector(std::vector<InventoryItem>& items, const User& user):
                UnclaimedTransactionPoolVisitor(),
                items_(items),
                user_(user){}
        ~UserUnclaimedTransactionCollector(){}

        User GetUser() const{
            return user_;
        }

        bool Visit(const UnclaimedTransactionPtr& utxo){
            if(utxo->GetUser() == GetUser())
                items_.push_back(InventoryItem(utxo));
            return true;
        }
    };

    void ServerSession::HandleGetUnclaimedTransactionsMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        GetUnclaimedTransactionsMessage* msg = task->GetMessage<GetUnclaimedTransactionsMessage>();

        User user = msg->GetUser();
        LOG(INFO) << "getting unclaimed transactions for " << user << "....";

        std::vector<InventoryItem> items;
        UserUnclaimedTransactionCollector collector(items, user);
        if(!UnclaimedTransactionPool::Accept(&collector)){
            LOG(WARNING) << "couldn't visit unclaimed transaction pool";
            //TODO: send not found?
            return;
        }

        if(items.empty()){
            LOG(WARNING) << "no unclaimed transactions found for user: " << user;
            session->Send(NotFoundMessage::NewInstance());
            return;
        }

        LOG(INFO) << "sending inventory of " << items.size() << " items";

        InventoryMessage* response = InventoryMessage::NewInstance(items);
        session->Send(response);
    }

    void ServerSession::HandleGetBlocksMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        GetBlocksMessage* msg = task->GetMessage<GetBlocksMessage>();

        Hash start = msg->GetHeadHash();
        Hash stop = msg->GetStopHash();

        std::vector<InventoryItem> items;
        if(stop.IsNull()){
            intptr_t amt = std::min(GetBlocksMessage::kMaxNumberOfBlocks, BlockChain::GetHead()->GetHeight());
            LOG(INFO) << "sending " << (amt + 1) << " blocks...";

            BlockPtr start_block = BlockChain::GetBlock(start);
            BlockPtr stop_block = BlockChain::GetBlock(start_block->GetHeight() > amt ? start_block->GetHeight() + amt : amt);

            for(uint32_t idx = start_block->GetHeight() + 1;
                idx <= stop_block->GetHeight();
                idx++){
                BlockPtr block = BlockChain::GetBlock(idx);
                LOG(INFO) << "adding " << block;
                items.push_back(InventoryItem(block));
            }
        }

        session->Send(InventoryMessage::NewInstance(items));
    }

    void ServerSession::HandleGetPeersMessage(HandleMessageTask* task){
        ServerSession* session = task->GetSession<ServerSession>();
        LOG(INFO) << "getting peers....";
        PeerList peers;
        /*
        TODO:
            if(!Server::GetPeers(peers)){
                LOG(ERROR) << "couldn't get list of peers from the server.";
                return;
            }
         */

        session->Send(PeerListMessage::NewInstance(peers));
    }

    void ServerSession::HandlePeerListMessage(HandleMessageTask* task){
        //TODO: implement ServerSession::HandlePeerListMessage(HandleMessageTask*);
    }
}