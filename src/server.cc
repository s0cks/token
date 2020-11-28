#include <uuid/uuid.h>
#include <algorithm>
#include <random>
#include <condition_variable>

#include "server.h"
#include "message.h"
#include "task.h"
#include "configuration.h"
#include "peer_session.h"
#include "proposal.h"

#include "block_pool.h"
#include "transaction_pool.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
    static uv_tcp_t handle_;
    static uv_async_t shutdown_;

    static std::mutex mutex_;
    static std::condition_variable cond_;
    static Server::State state_ = Server::State::kStopped;
    static UUID node_id_;
    static NodeAddress callback_;
    static PeerSession* peers_[Server::kMaxNumberOfPeers];
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

    bool Server::Accept(WeakObjectPointerVisitor* vis){
        LOCK_GUARD;
        for(size_t idx = 0; idx < Server::kMaxNumberOfPeers; idx++){
            if(peers_[idx] && !vis->Visit(&peers_[idx])){
                LOG(WARNING) << "cannot visit peer #" << idx;
                return false;
            }
        }
        return true;
    }

    void Server::SetState(Server::State state){
        LOCK;
        state_ = state;
        SIGNAL_ALL;
    }

    void Server::WaitForState(Server::State state){
        LOCK;
        while(state_ != state) WAIT;
    }

    Server::State Server::GetState() {
        LOCK_GUARD;
        return state_;
    }

    NodeAddress Server::GetCallbackAddress(){
        LOCK_GUARD;
        return callback_;
    }

    void Server::HandleTerminateCallback(uv_async_t* handle){
        uv_stop(handle->loop);
    }

    static inline void
    ConnectToPeers(const std::set<NodeAddress>& peers){
        for(auto& peer : peers){
            if(Server::IsConnectedTo(peer)){
                LOG(WARNING) << "already connected to peer: " << peer;
                continue;
            }

            if(!Server::ConnectTo(peer))
                LOG(WARNING) << "cannot connect to peer: " << peer;
        }
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
        ConnectToPeers(peers);
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

        Handle<ServerSession> session = ServerSession::NewInstance(stream->loop);
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

        Handle<Buffer> rbuff = session->GetReadBuffer();

        intptr_t offset = 0;
        std::vector<Handle<Message>> messages;
        do{
            int32_t mtype = rbuff->GetInt();
            int64_t  msize = rbuff->GetLong();

            switch(mtype) {
#define DEFINE_DECODE(Name) \
                case Message::MessageType::k##Name##MessageType:{ \
                    Handle<Message> msg = Name##Message::NewInstance(rbuff).CastTo<Message>(); \
                    LOG(INFO) << "decoded: " << msg << "(" << msize << " bytes)"; \
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

    bool Server::Broadcast(const Handle<Message>& msg){
        LOCK_GUARD;
        for(size_t idx = 0; idx < Server::kMaxNumberOfPeers; idx++){
            if(peers_[idx])
                peers_[idx]->Send(msg);
        }
        return true;
    }

    bool Server::ConnectTo(const NodeAddress& address){
        //-- Attention! --
        // this is not a memory leak, the memory will be freed upon
        // the session being closed
        Handle<PeerSession> session = PeerSession::NewInstance(uv_loop_new(), address);
        return session->Connect();
    }

    bool Server::IsConnectedTo(const NodeAddress& address){
        LOCK_GUARD;
        for(size_t idx = 0; idx < Server::kMaxNumberOfPeers; idx++){
            if(peers_[idx]){
                Peer peer = peers_[idx]->GetInfo();
                if(peer.GetAddress() == address)
                    return true;
            }
        }
        return false;
    }

    bool Server::IsConnectedTo(const UUID& uuid){
        LOCK_GUARD;
        for(size_t idx = 0; idx < Server::kMaxNumberOfPeers; idx++){
            if(peers_[idx]){
                Peer peer = peers_[idx]->GetInfo();
                if(peer.GetID() == uuid)
                    return true;
            }
        }
        return false;
    }

    Handle<PeerSession> Server::GetPeer(const UUID& uuid){
        LOCK_GUARD;
        for(size_t idx = 0; idx < Server::kMaxNumberOfPeers; idx++){
            if(peers_[idx]){
                Peer peer = peers_[idx]->GetInfo();
                if(peer.GetID() == uuid)
                    return peers_[idx];
            }
        }
        return nullptr;
    }

    bool Server::GetPeers(PeerList& peers){
        LOCK_GUARD;
        for(size_t idx = 0; idx < Server::kMaxNumberOfPeers; idx++){
            if(peers_[idx]){
                Peer peer = peers_[idx]->GetInfo();
                if(!peers.insert(peer).second){
                    LOG(WARNING) << "cannot add peer: " << peer;
                    return false;
                }
            }
        }
        return true;
    }

    int Server::GetNumberOfPeers(){
        LOCK_GUARD;
        int count = 0;
        for(size_t idx = 0; idx < Server::kMaxNumberOfPeers; idx++) {
            if(peers_[idx])
                count++;
        }
        return count;
    }

    bool Server::SavePeerList(){
        std::set<NodeAddress> peers;
        for(size_t idx = 0; idx < Server::kMaxNumberOfPeers; idx++) {
            if(peers_[idx])
                peers.insert(peers_[idx]->GetAddress());
        }

        if(!BlockChainConfiguration::SetPeerList(peers)){
            LOG(WARNING) << "cannot save the server peer list.";
            return false;
        }
        return true;
    }

    bool Server::RegisterPeer(const Handle<PeerSession>& session){
        LOCK_GUARD;
        for(size_t idx = 0; idx < Server::kMaxNumberOfPeers; idx++){
            if(!peers_[idx]){
                peers_[idx] = session;
                return SavePeerList();
            }
        }
        LOG(WARNING) << "cannot register peer: " << session->GetInfo();
        return false;
    }

    bool Server::UnregisterPeer(const Handle<PeerSession>& session){
        LOCK_GUARD;
        for(size_t idx = 0; idx < Server::kMaxNumberOfPeers; idx++){
            if(peers_[idx] && peers_[idx]->GetInfo() == session->GetInfo()){
                peers_[idx] = nullptr;
                return SavePeerList();
            }
        }
        LOG(WARNING) << "cannot unregister peer: " << session->GetInfo();
        return false;
    }

    void ServerSession::HandleNotFoundMessage(const Handle<HandleMessageTask>& task){
        //TODO: implement HandleNotFoundMessage
        LOG(WARNING) << "not implemented";
        return;
    }

    void ServerSession::HandleVersionMessage(const Handle<HandleMessageTask>& task){
        Handle<ServerSession> session = task->GetSession().CastTo<ServerSession>();
        //TODO:
        // - state check
        // - version check
        // - echo nonce
        session->Send(VersionMessage::NewInstance(Server::GetID()));
    }

    //TODO:
    // - verify nonce
    void ServerSession::HandleVerackMessage(const Handle<HandleMessageTask>& task){
        Handle<ServerSession> session = task->GetSession().CastTo<ServerSession>();
        Handle<VerackMessage> msg = task->GetMessage().CastTo<VerackMessage>();

        session->Send(VerackMessage::NewInstance(ClientType::kNode, Server::GetID(), Server::GetCallbackAddress()));

        LOG(INFO) << "State: " << session->GetState();
        if(session->IsConnecting()){
            session->SetState(Session::State::kConnected);
            if(msg->IsNode()){
                NodeAddress paddr = msg->GetCallbackAddress();
                LOG(INFO) << "peer connected from: " << paddr;
                if(!Server::IsConnectedTo(paddr)){
                    LOG(WARNING) << "couldn't find peer: " << msg->GetID() << ", connecting to peer " << paddr << "....";
                    if(!Server::ConnectTo(paddr)){
                        LOG(WARNING) << "couldn't connect to peer: " << paddr;
                    }
                }
            }
        }
    }

    void ServerSession::HandleGetDataMessage(const Handle<HandleMessageTask>& task){
        Handle<ServerSession> session = task->GetSession().CastTo<ServerSession>();
        Handle<GetDataMessage> msg = task->GetMessage().CastTo<GetDataMessage>();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "cannot get items from message";
            return;
        }

        std::vector<Handle<Message>> response;
        for(auto& item : items){
            Hash hash = item.GetHash();
            LOG(INFO) << "searching for: " << hash;
            if(item.ItemExists()){
                if(item.IsBlock()){
                    Handle<Block> block = nullptr;
                    if(BlockChain::HasBlock(hash)){
                        block = BlockChain::GetBlock(hash);
                    } else if(BlockPool::HasBlock(hash)){
                        block = BlockPool::GetBlock(hash);
                    } else{
                        LOG(WARNING) << "cannot find block: " << hash;
                        response.push_back(NotFoundMessage::NewInstance().CastTo<Message>());
                        break;
                    }

                    response.push_back(BlockMessage::NewInstance(block).CastTo<Message>());
                } else if(item.IsTransaction()){
                    if(!TransactionPool::HasTransaction(hash)){
                        LOG(WARNING) << "cannot find transaction: " << hash;
                        response.push_back(NotFoundMessage::NewInstance().CastTo<Message>());
                        break;
                    }

                    Handle<Transaction> tx = TransactionPool::GetTransaction(hash);
                    response.push_back(TransactionMessage::NewInstance(tx).CastTo<Message>());
                } else if(item.IsUnclaimedTransaction()){
                    if(!UnclaimedTransactionPool::HasUnclaimedTransaction(hash)){
                        LOG(WARNING) << "couldn't find unclaimed transaction: " << hash;
                        response.push_back(NotFoundMessage::NewInstance().CastTo<Message>());
                        break;
                    }

                    Handle<UnclaimedTransaction> utxo = UnclaimedTransactionPool::GetUnclaimedTransaction(hash);
                    response.push_back(UnclaimedTransactionMessage::NewInstance(utxo).CastTo<Message>());
                }
            } else{
                session->Send(NotFoundMessage::NewInstance());
                return;
            }
        }

        session->Send(response);
    }

    void ServerSession::HandlePrepareMessage(const Handle<HandleMessageTask>& task){
        Handle<ServerSession> session = task->GetSession().CastTo<ServerSession>();
        Handle<PrepareMessage> msg = task->GetMessage().CastTo<PrepareMessage>();

        Handle<Proposal> proposal = msg->GetProposal();
        /*if(ProposerThread::HasProposal()){
            session->Send(RejectedMessage::NewInstance(proposal, Server::GetID()));
            return;
        }*/

        //ProposerThread::SetProposal(proposal);
        proposal->SetPhase(Proposal::kVotingPhase);
        std::vector<Handle<Message>> response;
        if(!BlockPool::HasBlock(proposal->GetHash())){
            std::vector<InventoryItem> items = {
                    InventoryItem(InventoryItem::kBlock, proposal->GetHash())
            };
            response.push_back(GetDataMessage::NewInstance(items).CastTo<Message>());
        }
        response.push_back(PromiseMessage::NewInstance(proposal, Server::GetID()).CastTo<Message>());
        session->Send(response);
    }

    void ServerSession::HandlePromiseMessage(const Handle<HandleMessageTask>& task){}

    void ServerSession::HandleCommitMessage(const Handle<HandleMessageTask>& task){
        Handle<ServerSession> session = task->GetSession().CastTo<ServerSession>();
        Handle<CommitMessage> msg = task->GetMessage().CastTo<CommitMessage>();

        Proposal* proposal = msg->GetProposal();
        Hash hash = proposal->GetHash();

        if(!BlockPool::HasBlock(hash)){
            LOG(WARNING) << "couldn't find block " << hash << " in pool, rejecting request to commit proposal....";
            session->Send(RejectedMessage::NewInstance(proposal, Server::GetID()));
        } else{
            proposal->SetPhase(Proposal::kCommitPhase);
            session->Send(AcceptedMessage::NewInstance(proposal, Server::GetID()));
        }
    }

    void ServerSession::HandleAcceptedMessage(const Handle<HandleMessageTask>& task){
        Handle<AcceptedMessage> msg = task->GetMessage().CastTo<AcceptedMessage>();
        Handle<Proposal> proposal = msg->GetProposal();

        // validate proposal still?
        proposal->SetPhase(Proposal::kQuorumPhase);
    }

    void ServerSession::HandleRejectedMessage(const Handle<HandleMessageTask>& task){}

    void ServerSession::HandleBlockMessage(const Handle<HandleMessageTask>& task){
        Handle<BlockMessage> msg = task->GetMessage().CastTo<BlockMessage>();

        Block* block = msg->GetBlock();
        Hash hash = block->GetHash();

        if(!BlockPool::HasBlock(hash)){
            BlockPool::PutBlock(block);
            Server::Broadcast(InventoryMessage::NewInstance(block).CastTo<Message>());
        }
    }

    void ServerSession::HandleTransactionMessage(const Handle<HandleMessageTask>& task){
        Handle<TransactionMessage> msg = task->GetMessage().CastTo<TransactionMessage>();

        Handle<Transaction> tx = msg->GetTransaction();
        Hash hash = tx->GetHash();

        LOG(INFO) << "received transaction: " << hash;
        if(!TransactionPool::HasTransaction(hash)){
            TransactionPool::PutTransaction(tx);
            Server::Broadcast(InventoryMessage::NewInstance(tx).CastTo<Message>());
        }
    }

    void ServerSession::HandleUnclaimedTransactionMessage(const Handle<HandleMessageTask>& task){

    }

    void ServerSession::HandleInventoryMessage(const Handle<HandleMessageTask>& task){
        Handle<ServerSession> session = task->GetSession().CastTo<ServerSession>();
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

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            if(utxo->GetUser() == GetUser())
                items_.push_back(InventoryItem(utxo));
            return true;
        }
    };

    void ServerSession::HandleGetUnclaimedTransactionsMessage(const Token::Handle<Token::HandleMessageTask>& task){
        Handle<ServerSession> session = task->GetSession().CastTo<ServerSession>();
        Handle<GetUnclaimedTransactionsMessage> msg = task->GetMessage().CastTo<GetUnclaimedTransactionsMessage>();

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
        session->SendInventory(items);
    }

    void ServerSession::HandleGetBlocksMessage(const Handle<HandleMessageTask>& task){
        Handle<ServerSession> session = task->GetSession().CastTo<ServerSession>();
        Handle<GetBlocksMessage> msg = task->GetMessage().CastTo<GetBlocksMessage>();

        Hash start = msg->GetHeadHash();
        Hash stop = msg->GetStopHash();

        std::vector<InventoryItem> items;
        if(stop.IsNull()){
            intptr_t amt = std::min(GetBlocksMessage::kMaxNumberOfBlocks, BlockChain::GetHead()->GetHeight());
            LOG(INFO) << "sending " << (amt + 1) << " blocks...";

            Handle<Block> start_block = BlockChain::GetBlock(start);
            Handle<Block> stop_block = BlockChain::GetBlock(start_block->GetHeight() > amt ? start_block->GetHeight() + amt : amt);

            for(uint32_t idx = start_block->GetHeight() + 1;
                idx <= stop_block->GetHeight();
                idx++){
                Handle<Block> block = BlockChain::GetBlock(idx);
                LOG(INFO) << "adding " << block;
                items.push_back(InventoryItem(block));
            }
        }

        session->Send(InventoryMessage::NewInstance(items));
    }

    void ServerSession::HandleGetPeersMessage(const Handle<HandleMessageTask>& task){
        Handle<ServerSession> session = task->GetSession().CastTo<ServerSession>();
        Handle<GetPeersMessage> msg = task->GetMessage().CastTo<GetPeersMessage>();

        LOG(INFO) << "getting peers....";

        PeerList peers;
        if(!Server::GetPeers(peers)){
            LOG(ERROR) << "couldn't get list of peers from the server.";
            return;
        }

        session->Send(PeerListMessage::NewInstance(peers));
    }

    void ServerSession::HandlePeerListMessage(const Handle<HandleMessageTask>& task){
        //TODO: implement ServerSession::HandlePeerListMessage(const Handle<HandleMessageTask>&);
    }
}