#include <queue>

#include "peer.h"
#include "task.h"
#include "server.h"
#include "async_task.h"
#include "configuration.h"
#include "block_discovery.h"

namespace Token{
    void PeerSession::OnShutdown(uv_async_t* handle){
        //PeerSession* session = (PeerSession*)handle->data;
        //TODO: session->Disconnect();
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

    void PeerSession::OnConnect(uv_connect_t* conn, int status){
        PeerSession* session = (PeerSession*)conn->data;
        if(status != 0){
            LOG(WARNING) << "client accept error: " << uv_strerror(status);
            //TODO: session->Disconnect();
            return;
        }

        session->SetState(Session::kConnecting);

        Block* head = BlockChain::GetHead();
        session->Send(VersionMessage::NewInstance(ClientType::kNode, Server::GetID(), head->GetHeader()));
        delete head;

        if((status = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
            LOG(WARNING) << "client read error: " << uv_strerror(status);
            //TODO: session->Disconnect();
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

    static std::mutex session_queue_mutex_;
    static std::condition_variable session_queue_cond_;
    static std::queue<NodeAddress> session_queue_;

    static inline bool Queue(const NodeAddress& paddress){
        std::unique_lock<std::mutex> lock(session_queue_mutex_);
        if((int32_t)session_queue_.size() == BlockChainConfiguration::GetMaxNumberOfPeers())
            return false;
        session_queue_.push(paddress);
        session_queue_cond_.notify_one();
        return true;
    }

    static inline NodeAddress Dequeue(){
        std::unique_lock<std::mutex> lock(session_queue_mutex_);
        while(session_queue_.empty())
            session_queue_cond_.wait(lock);
        NodeAddress next = session_queue_.front();
        session_queue_.pop();
        return next;
    }

#define FOR_EACH_PEER_SESSION_MANAGER_STATE(V) \
    V(Starting)                                \
    V(Idle)                                    \
    V(Connected)                               \
    V(Stopping)                               \
    V(Stopped)

#define FOR_EACH_PEER_SESSION_MANAGER_STATUS(V) \
    V(Ok)                                       \
    V(Warning)                                  \
    V(Error)

    class PeerSessionThread{
    public:
        enum State{
#define DEFINE_STATE(Name) k##Name,
            FOR_EACH_PEER_SESSION_MANAGER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
#define DEFINE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_PEER_SESSION_MANAGER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
                default:
                    stream << "Unknown";
                    return stream;
            }
        }

        enum Status{
#define DEFINE_STATUS(Name) k##Name,
            FOR_EACH_PEER_SESSION_MANAGER_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
        };

        friend std::ostream& operator<<(std::ostream& stream, const Status& status){
            switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_PEER_SESSION_MANAGER_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
            }
        }
    private:
        pthread_t thread_;
        std::mutex mutex_;
        std::condition_variable cond_;
        State state_;
        Status status_;
        uv_loop_t* loop_;
        std::shared_ptr<PeerSession> session_;

        void SetCurrentSession(const std::shared_ptr<PeerSession>& session){
            std::unique_lock<std::mutex> guard(mutex_);
            session_ = session;
        }

        std::shared_ptr<PeerSession> CreateNewSession(const NodeAddress& address){
            std::unique_lock<std::mutex> guard(mutex_);
            session_ = std::make_shared<PeerSession>(GetLoop(), address);
            return session_;
        }

        static void* HandleThread(void* data){
            PeerSessionThread* thread = (PeerSessionThread*)data;
            thread->SetState(State::kStarting);

            // Startup logic here

            thread->SetState(State::kIdle);
            while(!thread->IsStopping()){
                NodeAddress paddr = Dequeue();
                LOG(INFO) << "connecting to peer: " << paddr;
                std::shared_ptr<PeerSession> session = thread->CreateNewSession(paddr);
                thread->SetState(State::kConnected); //TODO: fix this is not "connected" it's connecting...
                if(!session->Connect()){
                    LOG(WARNING) << "couldn't connect to peer " << paddr << ", rescheduling...";
                    //TODO: reschedule peer connection
                    continue;
                }
                LOG(INFO) << "disconnected from peer: " << paddr;
            }

            pthread_exit(0);
        }
    public:
        PeerSessionThread(uv_loop_t* loop=uv_loop_new()):
            thread_(),
            mutex_(),
            cond_(),
            state_(),
            status_(),
            loop_(loop){}
        ~PeerSessionThread(){
            if(loop_)
                uv_loop_delete(loop_);
        }

        uv_loop_t* GetLoop() const{
            return loop_;
        }

        State GetState(){
            std::lock_guard<std::mutex> guard(mutex_);
            return state_;
        }

        void SetState(const State& state){
            std::lock_guard<std::mutex> guard(mutex_);
            state_ = state;
        }

        Status GetStatus(){
            std::lock_guard<std::mutex> guard(mutex_);
            return status_;
        }

        void SetStatus(const Status& status){
            std::lock_guard<std::mutex> guard(mutex_);
            status_ = status;
        }

        std::shared_ptr<PeerSession> GetCurrentSession(){
            std::lock_guard<std::mutex> guard(mutex_);
            return session_;
        }

#define DEFINE_CHECK(Name) \
        bool Is##Name(){ return GetState() == State::k##Name; }
        FOR_EACH_PEER_SESSION_MANAGER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

#define DEFINE_CHECK(Name) \
        bool Is##Name(){ return GetStatus() == Status::k##Name; }
        FOR_EACH_PEER_SESSION_MANAGER_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK

        bool Start(){
            int result;
            pthread_attr_t thread_attr;
            if((result = pthread_attr_init(&thread_attr)) != 0){
                LOG(WARNING) << "couldn't initialize the thread attributes: " << strerror(result);
                return false;
            }

            if((result = pthread_create(&thread_, &thread_attr, &HandleThread, this)) != 0){
                LOG(WARNING) << "couldn't start the session thread: " << strerror(result);
                return false;
            }

            if((result = pthread_attr_destroy(&thread_attr)) != 0){
                LOG(WARNING) << "couldn't destroy the thread attributes: " << strerror(result);
                return false;
            }
            return true;
        }
    };

    static std::mutex mutex_;
    static std::condition_variable cond_;
    static std::unique_ptr<PeerSessionThread>* threads_;

#define LOCK_GUARD std::lock_guard<std::mutex> __guard__(mutex_)
#define LOCK std::unique_lock<std::mutex> __lock__(mutex_)
#define WAIT cond_.wait(__lock__)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    bool PeerSessionManager::Initialize(){
        LOCK_GUARD;

        int32_t nworkers = BlockChainConfiguration::GetMaxNumberOfPeers();
        threads_ = new std::unique_ptr<PeerSessionThread>[nworkers];
        for(int32_t idx = 0;
            idx < nworkers;
            idx++){
#ifdef TOKEN_DEBUG
            LOG(INFO) << "starting peer session thread #" << idx << "....";
#endif//TOKEN_DEBUG
            threads_[idx] = std::unique_ptr<PeerSessionThread>(new PeerSessionThread());
            threads_[idx]->Start();
        }

        PeerList peers;
        if(!BlockChainConfiguration::GetPeerList(peers)){
            LOG(WARNING) << "couldn't get list of peers from the configuration.";
            return true;
        }

        for(auto& it : peers)
            Queue(it);
        return true;
    }

    bool PeerSessionManager::Shutdown(){
        LOG(WARNING) << "PeerSessionManager::Shutdown() - Not Implemented Yet."; //TODO: Implement PeerSessionManager::Shutdown;
        return false;
    }

    std::shared_ptr<PeerSession> PeerSessionManager::GetSession(const NodeAddress& address){
        int32_t nworkers = BlockChainConfiguration::GetMaxNumberOfPeers();

        LOCK_GUARD;
        for(int32_t idx = 0; idx < nworkers; idx++){
            std::unique_ptr<PeerSessionThread>& thread = threads_[idx];
            if(thread->IsConnected()){
                std::shared_ptr<PeerSession> session = thread->GetCurrentSession();
                if(session->GetAddress() == address)
                    return session;
            }
        }
        return nullptr;
    }

    std::shared_ptr<PeerSession> PeerSessionManager::GetSession(const UUID& uuid){
        int32_t nworkers = BlockChainConfiguration::GetMaxNumberOfPeers();

        LOCK_GUARD;
        for(int32_t idx = 0; idx < nworkers; idx++){
            std::unique_ptr<PeerSessionThread>& thread = threads_[idx];
            if(thread->IsConnected()){
                std::shared_ptr<PeerSession> session = thread->GetCurrentSession();
                if(session->GetID() == uuid)
                    return session;
            }
        }
        return nullptr;
    }

    int32_t PeerSessionManager::GetNumberOfConnectedPeers(){
        int32_t nworkers = BlockChainConfiguration::GetMaxNumberOfPeers();
        int32_t count = 0;

        LOCK_GUARD;
        for(int32_t idx = 0; idx < nworkers; idx++){
            if(threads_[idx]->IsConnected())
                count++;
        }
        return count;
    }

    bool PeerSessionManager::ConnectTo(const NodeAddress& address){
        if(IsConnectedTo(address))
            return false;
        return Queue(address);
    }

    bool PeerSessionManager::IsConnectedTo(const UUID& uuid){
        int32_t nworkers = BlockChainConfiguration::GetMaxNumberOfPeers();
        LOCK_GUARD;
        for(int32_t idx = 0; idx < nworkers; idx++){
            std::unique_ptr<PeerSessionThread>& thread = threads_[idx];
            if(thread->IsConnected()){
                std::shared_ptr<PeerSession> session = thread->GetCurrentSession();
                if(session->GetID() == uuid)
                    return true;
            }
        }
        return false;
    }

    bool PeerSessionManager::IsConnectedTo(const NodeAddress& address){
        int32_t nworkers = BlockChainConfiguration::GetMaxNumberOfPeers();
        LOCK_GUARD;
        for(int32_t idx = 0; idx < nworkers; idx++){
            std::unique_ptr<PeerSessionThread>& thread = threads_[idx];
            if(thread->IsConnected()){
                std::shared_ptr<PeerSession> session = thread->GetCurrentSession();
                if(session->GetAddress() == address)
                    return true;
            }
        }
        return false;
    }

    void PeerSessionManager::BroadcastPrepare(){
        int32_t nworkers = BlockChainConfiguration::GetMaxNumberOfPeers();
        LOCK_GUARD;
        for(int32_t idx = 0; idx < nworkers; idx++){
            std::unique_ptr<PeerSessionThread>& thread = threads_[idx];
            if(thread->IsConnected())
                thread->GetCurrentSession()->SendPrepare();
        }
    }

    void PeerSessionManager::BroadcastCommit(){
        int32_t nworkers = BlockChainConfiguration::GetMaxNumberOfPeers();
        LOCK_GUARD;
        for(int32_t idx = 0; idx < nworkers; idx++){
            std::unique_ptr<PeerSessionThread>& thread = threads_[idx];
            if(thread->IsConnected())
                thread->GetCurrentSession()->SendCommit();
        }
    }
}