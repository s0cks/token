#include <uuid/uuid.h>
#include <algorithm>
#include <random>
#include <condition_variable>

#include "server.h"
#include "message.h"
#include "task.h"
#include "configuration.h"
#include "peer.h"
#include "byte_buffer.h"
#include "proposal.h"

namespace Token{
    static uv_tcp_t handle_;
    static uv_async_t aterm_;

    static std::mutex mutex_;
    static std::condition_variable cond_;
    static Server::State state_ = Server::State::kStopped;
    static UUID node_id_;
    static std::map<UUID, PeerSession*> peers_;

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

    void Server::HandleTerminateCallback(uv_async_t* handle){
        uv_stop(handle->loop);
    }

    void Server::HandleThread(uword parameter){
        LOG(INFO) << "starting server...";
        SetState(State::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &aterm_, &HandleTerminateCallback);

        struct sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", FLAGS_port, &addr);
        uv_tcp_init(loop, GetHandle());
        uv_tcp_keepalive(GetHandle(), 1, 60);
        int err;
        if((err = uv_tcp_bind(GetHandle(), (const struct sockaddr*)&addr, 0)) != 0){
            LOG(ERROR) << ""; //TODO:
            pthread_exit(0);
        }

        if((err = uv_listen((uv_stream_t*)GetHandle(), 100, &OnNewConnection)) != 0){
            LOG(ERROR) << ""; //TODO:
            pthread_exit(0);
        }

        LOG(INFO) << "server " << GetID() << " listening @" << FLAGS_port;
        SetState(State::kRunning);
        uv_run(loop, UV_RUN_DEFAULT);

        uv_loop_close(loop);
        pthread_exit(0);
    }

    void Server::OnNewConnection(uv_stream_t* stream, int status){
        if(status != 0){
            LOG(ERROR) << "connection error: " << uv_strerror(status);
            return;
        }

        ServerSession* session = new ServerSession();
        session->SetState(Session::kConnecting);

        uv_tcp_init(stream->loop, (uv_tcp_t*)session->GetStream());
        LOG(INFO) << "client is connecting...";
        if((status = uv_accept(stream, (uv_stream_t*)session->GetStream())) != 0){
            LOG(ERROR) << "client accept error: " << uv_strerror(status);
            return;
        }

        LOG(INFO) << "client connected";
        if((status = uv_read_start(session->GetStream(), &Session::AllocBuffer, &OnMessageReceived)) != 0){
            LOG(ERROR) << "client read error: " << uv_strerror(status);
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
    }

    bool Server::Broadcast(const Handle<Message>& msg){
        LOCK_GUARD;
        for(auto& it : peers_) it.second->Send(msg);
        return true;
    }

#define CONFIG_KEY_SERVER "Server"
#define CONFIG_KEY_ID "Id"
#define CONFIG_KEY_PEERS "Peers"

    static inline bool
    LoadConfiguration(libconfig::Setting& config){
        LOG(INFO) << "loading the server configuration....";
        if(!config.exists(CONFIG_KEY_ID)){
            node_id_ = UUID();
            config.add(CONFIG_KEY_ID, libconfig::Setting::TypeString) = node_id_.ToString();
            BlockChainConfiguration::SaveConfiguration();
        } else{
            std::string node_id;
            config.lookupValue(CONFIG_KEY_ID, node_id);
            node_id_ = UUID(node_id);
        }
        return true;
    }

    static inline bool
    LoadPeers(libconfig::Setting& config, std::set<NodeAddress>& peers){
        LOG(INFO) << "loading peers....";
        if(!config.exists(CONFIG_KEY_PEERS)){
            config.add(CONFIG_KEY_PEERS, libconfig::Setting::TypeArray);
            BlockChainConfiguration::SaveConfiguration();
        } else{
            libconfig::Setting& pcfg = config.lookup(CONFIG_KEY_PEERS);
            auto iter = pcfg.begin();
            while(iter != pcfg.end()){
                peers.insert(NodeAddress(std::string(iter->c_str())));
                iter++;
            }
        }
        return true;
    }

    bool Server::Initialize(){
        LOG(INFO) << "initializing the server....";
        LOCK_GUARD;
        libconfig::Setting& config = BlockChainConfiguration::GetProperty(CONFIG_KEY_SERVER, libconfig::Setting::TypeGroup);
        if(!LoadConfiguration(config)){
            LOG(WARNING) << "couldn't load server information from the configuration.";
            return false;
        }

        std::set<NodeAddress> peers;
        if(!LoadPeers(config, peers)){
            LOG(WARNING) << "couldn't load peers from the configuration.";
            return false;
        }
        if(!FLAGS_peer.empty()){
            peers.insert(NodeAddress(FLAGS_peer));
        }

        for(auto& peer : peers){
            if(!HasPeer(peer)){
                if(!ConnectTo(peer)){
                    LOG(WARNING) << "cannot connect to peer: " << peer;
                }
            } else{
                LOG(WARNING) << "already connected to peer: " << peer;
            }
        }
        return true;
    }

    bool Server::HasPeer(const UUID& uuid){
        LOCK_GUARD;
        for(auto& it : peers_){
            PeerSession* peer = it.second;
            if(peer->GetID() == uuid)
                return true;
        }
        return false;
    }

    bool Server::ConnectTo(const NodeAddress &address){
        //-- Attention! --
        // this is not a memory leak, the memory will be freed upon
        // the session being closed
        return (new PeerSession(address))->Connect();
    }

    bool Server::IsConnectedTo(const NodeAddress& address){
        LOCK_GUARD;
        for(auto& it : peers_){
            PeerSession* peer = it.second;
            LOG(INFO) << "checking: " << peer->GetAddress() << " == " << address;
            if(peer->GetAddress() == address){
                return true;
            }
        }
        return false;
    }

    PeerSession* Server::GetPeer(const UUID& uuid){
        LOCK_GUARD;
        for(auto& it : peers_){
            PeerSession* peer = it.second;
            if(peer->GetID() == uuid)
                return peer;
        }
        return nullptr;
    }

    bool Server::GetPeers(PeerList& peers){
        LOCK_GUARD;
        for(auto& it : peers_){
            PeerSession* psession = it.second;
            Peer peer(psession->GetID(), psession->GetAddress());
            if(!(peers.insert(peer).second)){
                LOG(WARNING) << "cannot add peer: " << peer;
                return false;
            }
        }
        return true;
    }

    int Server::GetNumberOfPeers(){
        LOCK_GUARD;
        return peers_.size();
    }

    static inline bool
    SavePeers(){
        LOG(INFO) << "saving peers....";
        libconfig::Setting& config = BlockChainConfiguration::GetProperty(CONFIG_KEY_SERVER, libconfig::Setting::TypeGroup);
        if(config.exists(CONFIG_KEY_PEERS)){
            config.remove(CONFIG_KEY_PEERS);
        }

        libconfig::Setting& peers = config.add(CONFIG_KEY_PEERS, libconfig::Setting::TypeArray);
        for(auto peer : peers_){
            NodeAddress paddress = peer.second->GetAddress();
            peers.add(libconfig::Setting::TypeString) = paddress.ToString();
        }
        return BlockChainConfiguration::SaveConfiguration();
    }

    bool Server::HasPeer(const NodeAddress& address){
        for(auto& it : peers_){
            PeerSession* peer = it.second;
            LOG(INFO) << "checking: " << peer->GetAddress();
            if(peer->GetAddress() == address){
                return true;
            }
        }
        return false;
    }

    bool Server::RegisterPeer(PeerSession* session){
        LOCK_GUARD;
        if(!(peers_.insert({ session->GetID(), session }).second)){
            LOG(WARNING) << "cannot register peer: " << session->GetID();
            return false;
        }
        return SavePeers();
    }

    bool Server::UnregisterPeer(PeerSession* session){
        LOCK_GUARD;
        if(peers_.erase(session->GetID()) != 1){
            LOG(WARNING) << "cannot unregister peer: " << session->GetID();
            return false;
        }
        return SavePeers();
    }

    void ServerSession::HandleNotFoundMessage(const Handle<HandleMessageTask>& task){
        //TODO: implement HandleNotFoundMessage
        LOG(WARNING) << "not implemented";
        return;
    }

    void ServerSession::HandleVersionMessage(const Handle<HandleMessageTask>& task){
        ServerSession* session = (ServerSession*)task->GetSession();
        //TODO:
        // - state check
        // - version check
        // - echo nonce
        session->Send(VersionMessage::NewInstance(Server::GetID()));
    }

    //TODO:
    // - verify nonce
    void ServerSession::HandleVerackMessage(const Handle<HandleMessageTask>& task){
        ServerSession* session = (ServerSession*)task->GetSession();
        Handle<VerackMessage> msg = task->GetMessage().CastTo<VerackMessage>();

        NodeAddress callback("127.0.0.1", FLAGS_port); //TODO: obtain address dynamically
        session->Send(VerackMessage::NewInstance(ClientType::kNode, Server::GetID(), callback));

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
        ServerSession* session = (ServerSession*)task->GetSession();
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
                Send(NotFoundMessage::NewInstance());
                return;
            }
        }

        session->Send(response);
    }

    void ServerSession::HandlePrepareMessage(const Handle<HandleMessageTask>& task){
        ServerSession* session = (ServerSession*)task->GetSession();
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
        ServerSession* session = (ServerSession*)task->GetSession();
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
        ServerSession* session = (ServerSession*)task->GetSession();
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
        ServerSession* session = (ServerSession*)task->GetSession();
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
        ServerSession* session = (ServerSession*)task->GetSession();
        Handle<GetBlocksMessage> msg = task->GetMessage().CastTo<GetBlocksMessage>();

        Hash start = msg->GetHeadHash();
        Hash stop = msg->GetStopHash();

        std::vector<InventoryItem> items;
        if(stop.IsNull()){
            intptr_t amt = std::min(GetBlocksMessage::kMaxNumberOfBlocks, BlockChain::GetHead().GetHeight());
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
        ServerSession* session = (ServerSession*)task->GetSession();
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