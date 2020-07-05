#include <algorithm>
#include <random>

#include "node/node.h"
#include "node/message.h"
#include "node/task.h"
#include "block_miner.h"
#include "configuration.h"

namespace Token{
#define SCHEDULE(Loop, Name, ...)({ \
    uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));\
    work->data = new Name##Task(__VA_ARGS__); \
    uv_queue_work((Loop), work, Handle##Name##Task, After##Name##Task); \
})

    static pthread_t thread_ = 0;
    static uv_tcp_t handle_;
    static std::string node_id_ = GenerateNonce();
    static std::map<std::string, PeerSession*> peers_ = std::map<std::string, PeerSession*>();
    static pthread_rwlock_t rwlock_ = PTHREAD_RWLOCK_INITIALIZER;
    static Node::State state_ = Node::State::kStopped;
    static pthread_mutex_t state_mutex_ = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t state_cond_ = PTHREAD_COND_INITIALIZER;

    void Node::LoadPeers(){
#ifdef TOKEN_ENABLE_DEBUG
        LOG(INFO) << "loading peers...";
#endif//TOKEN_ENABLE_DEBUG

        libconfig::Setting& peers = BlockChainConfiguration::GetProperty("Peers", libconfig::Setting::TypeArray);
        auto iter = peers.begin();
        while(iter != peers.end()){
            NodeAddress address((*iter));

#ifdef TOKEN_ENABLE_DEBUG
            LOG(INFO) << "loaded peer: " << address;
#endif//TOKEN_ENABLE_DEBUG

            if(!ConnectTo(address)) LOG(WARNING) << "couldn't connect to peer: " << address;
        }
    }

    void Node::SavePeers(){
        libconfig::Setting& root = BlockChainConfiguration::GetRoot();
        if(root.exists("Peers")) root.remove("Peers");

        libconfig::Setting& peers = root.add("Peers", libconfig::Setting::TypeArray);
        for(auto& it : peers_){
            NodeAddress address = it.second->GetAddress();
            peers.add(libconfig::Setting::TypeString) = address.ToString();
        }

        BlockChainConfiguration::SaveConfiguration();
    }

    uv_tcp_t* Node::GetHandle(){
        return &handle_;
    }

    std::string Node::GetNodeID(){
        return node_id_;
    }

    bool Node::Start(){
        if(!IsStopped()) return false;
        LoadPeers();
        return pthread_create(&thread_, NULL, &NodeThread, NULL) == 0;
    }

    bool Node::Shutdown(){
        if(!IsRunning()) return false;
        return pthread_join(thread_, NULL) == 0;
    }

    bool Node::WaitForShutdown(){
        WaitForState(State::kStopped);
        return true;
    }

    void Node::SetState(Node::State state){
        pthread_mutex_trylock(&state_mutex_);
        state_ = state;
        pthread_cond_signal(&state_cond_);
        pthread_mutex_unlock(&state_mutex_);
    }

    void Node::WaitForState(Node::State state){
        pthread_mutex_trylock(&state_mutex_);
        while(state_ != state) pthread_cond_wait(&state_cond_, &state_mutex_);
        pthread_mutex_unlock(&state_mutex_);
    }

    Node::State Node::GetState() {
        pthread_mutex_trylock(&state_mutex_);
        Node::State state = state_;
        pthread_mutex_unlock(&state_mutex_);
        return state;
    }

    uint32_t Node::GetNumberOfPeers(){
        pthread_rwlock_tryrdlock(&rwlock_);
        uint32_t peers = peers_.size();
        pthread_rwlock_unlock(&rwlock_);
        return peers;
    }

    void* Node::NodeThread(void* ptr){
        SetState(State::kStarting);
        uv_loop_t* loop = uv_loop_new();

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

        SetState(State::kRunning);
        LOG(INFO) << "server listening: @" << FLAGS_port;
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(0);
    }

    void Node::OnNewConnection(uv_stream_t* stream, int status){
        LOG(INFO) << "client is connecting...";
        if(status != 0){
            LOG(ERROR) << "connection error: " << uv_strerror(status);
            return;
        }

        NodeSession* session = new NodeSession();
        uv_tcp_init(stream->loop, (uv_tcp_t*)session->GetStream());

        if((status = uv_accept(stream, (uv_stream_t*)session->GetStream())) != 0){
            LOG(ERROR) << "client accept error: " << uv_strerror(status);
            return;
        }

        if((status = uv_read_start(session->GetStream(), &Session::AllocBuffer, &OnMessageReceived)) != 0){
            LOG(ERROR) << "client read error: " << uv_strerror(status);
            return;
        }
    }

    void Node::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        NodeSession* session = (NodeSession*)stream->data;
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
        do{
            uint32_t mtype = 0;
            memcpy(&mtype, &buff->base[offset + Message::kTypeOffset], Message::kTypeLength);

            uint64_t msize = 0;
            memcpy(&msize, &buff->base[offset + Message::kSizeOffset], Message::kSizeLength);

            Message* msg = nullptr;
            if(!(msg = Message::Decode(static_cast<Message::MessageType>(mtype), msize, (uint8_t*)&buff->base[offset + Message::kDataOffset]))){
                LOG(WARNING) << "couldn't decode message of type := " << mtype << " and of size := " << msize << ", at offset := " << offset;
                continue;
            }

            LOG(INFO) << "decoded message: " << msg->ToString();
            messages.push_back(msg);

            offset += (msize + Message::kHeaderSize);
        } while((offset + Message::kHeaderSize) < nread);

        for(size_t idx = 0; idx < messages.size(); idx++){
            Message* msg = messages[idx];
            HandleMessageTask* task = new HandleMessageTask(session, msg);
            switch(msg->GetMessageType()){
#define DEFINE_HANDLER_CASE(Name) \
            case Message::k##Name##MessageType: \
                Handle##Name##Message(task); \
                break;
                FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE);
#undef DEFINE_HANDLER_CASE
                case Message::kUnknownMessageType:
                default: //TODO: handle properly
                    break;
            }

            delete task;
        }
    }

    void Node::HandleGetDataMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        GetDataMessage* msg = (GetDataMessage*)task->GetMessage();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "cannot get items from message";
            return;
        }

        std::vector<Message*> data;
        for(auto& item : items){
            uint256_t hash = item.GetHash();
            if(item.ItemExists()){
                if(item.IsBlock()){
                    Block* block = nullptr;
                    if((block = BlockChain::GetBlockData(hash))){
                        LOG(INFO) << hash << " found in block chain!";
                        data.push_back(BlockMessage::NewInstance(block));
                        continue;
                    }

                    if((block = BlockPool::GetBlock(hash))){
                        LOG(INFO) << hash << " found in block pool!";
                        data.push_back(BlockMessage::NewInstance(block));
                        continue;
                    }

                    LOG(WARNING) << "couldn't find: " << hash << "!";
                    return;
                } else if(item.IsTransaction()){
                    Transaction* tx;
                    if((tx = TransactionPool::GetTransaction(hash))){
                        LOG(INFO) << hash << " found in transaction pool!";
                        data.push_back(TransactionMessage::NewInstance(tx));
                        continue;
                    }

                    LOG(WARNING) << "couldn't find: " << hash << "!";
                    return;
                }
            } else{
                LOG(WARNING) << "item doesn't exist: " << hash << "!";
                return;
            }
        }
        session->Send(data);
    }

    void Node::HandleVersionMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        //TODO:
        // - state check
        // - version check
        // - echo nonce
        session->Send(VersionMessage::NewInstance(GetNodeID()));
    }

    void Node::HandleVerackMessage(HandleMessageTask* task){
        VerackMessage* msg = (VerackMessage*)task->GetMessage();
        NodeSession* session = (NodeSession*)task->GetSession();

        NodeAddress paddr = msg->GetCallbackAddress();

        //TODO:
        // - verify nonce
        LOG(INFO) << "client connected!";
        session->Send(VerackMessage::NewInstance(GetNodeID(), NodeAddress("127.0.0.1", FLAGS_port))); //TODO: obtain address dynamically
        session->SetState(NodeSession::kConnected);
        if(!HasPeer(msg->GetID())){
            LOG(WARNING) << "connecting to peer: " << paddr << "....";
            if(!Node::ConnectTo(paddr)){
                LOG(WARNING) << "couldn't connect to peer: " << paddr;
            }
        }
    }

    void Node::HandlePrepareMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        PrepareMessage* msg = (PrepareMessage*)task->GetMessage();

        Proposal* proposal = msg->GetProposal();
        if(!proposal->IsValid()){
            LOG(WARNING) << "proposal " << (*proposal) << " is invalid!";
            return;
        }

        BlockMiner::SetProposal(proposal);
        session->Send(PromiseMessage::NewInstance(Node::GetNodeID(), (*proposal)));
    }

    void Node::HandlePromiseMessage(HandleMessageTask* task){}

    void Node::HandleCommitMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        CommitMessage* msg = (CommitMessage*)task->GetMessage();

        uint32_t height = msg->GetHeight();
        uint256_t hash = msg->GetHash();

        Block* block;
        if(!(block = BlockPool::GetBlock(hash))){
            LOG(WARNING) << "couldn't find block " << hash << " from pool waiting....";
            goto exit;
        }

        BlockChain::Append(block);
        BlockPool::RemoveBlock(hash);

        session->Send(AcceptedMessage::NewInstance(GetNodeID(), Proposal(msg->GetNodeID(), height, hash)));
    exit:
        return;
    }

    void Node::HandleBlockMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        BlockMessage* msg = (BlockMessage*)task->GetMessage();

        Block* block = msg->GetBlock();
        uint256_t hash = block->GetHash();

        BlockPool::PutBlock(block);
        session->OnHash(hash);
    }

    void Node::HandleTransactionMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        TransactionMessage* msg = (TransactionMessage*)task->GetMessage();

        Transaction* tx = msg->GetTransaction();
        uint256_t hash = tx->GetHash();

        if(!TransactionPool::PutTransaction(tx)){
            LOG(WARNING) << "couldn't add transaction to pool: " << hash;
            return;
        }

        session->OnHash(hash);
    }

    void Node::HandleAcceptedMessage(HandleMessageTask* task){}

    void Node::HandleRejectedMessage(HandleMessageTask* task){}

    void Node::HandleInventoryMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        InventoryMessage* msg = (InventoryMessage*)task->GetMessage();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "couldn't get items from inventory message";
            return;
        }

        auto item_exists = [](InventoryItem item){
            return item.ItemExists();
        };
        items.erase(std::remove_if(items.begin(), items.end(), item_exists), items.end());

        for(auto& item : items) LOG(INFO) << "downloading " << item << "....";
        if(!items.empty()) session->Send(GetDataMessage::NewInstance(items));
    }

    void Node::HandleGetBlocksMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        GetBlocksMessage* msg = (GetBlocksMessage*)task->GetMessage();

        uint256_t start = msg->GetHeadHash();
        uint256_t stop = msg->GetStopHash();

        std::vector<InventoryItem> items;
        if(stop.IsNull()){
            uint32_t amt = std::min(GetBlocksMessage::kMaxNumberOfBlocks, BlockChain::GetHead().GetHeight());
            LOG(INFO) << "sending " << (amt + 1) << " blocks...";

            BlockHeader start_block = BlockChain::GetBlock(start);
            BlockHeader stop_block = BlockChain::GetBlock(start_block.GetHeight() > amt ? start_block.GetHeight() + amt : amt);

            for(uint32_t idx = start_block.GetHeight() + 1;
                        idx <= stop_block.GetHeight();
                        idx++){
                BlockHeader block = BlockChain::GetBlock(idx);
                LOG(INFO) << "adding " << block;
                items.push_back(InventoryItem(block));
            }
        }

        session->Send(InventoryMessage::NewInstance(items));
    }

    bool Node::Broadcast(Message* msg){
        pthread_rwlock_trywrlock(&rwlock_);
        for(auto& it : peers_) it.second->Send(msg);
        pthread_rwlock_unlock(&rwlock_);
        return true;
    }

    void Node::RegisterPeer(const std::string& node_id, PeerSession* peer){
        pthread_rwlock_trywrlock(&rwlock_);
        peers_.insert({ node_id, peer });
        SavePeers();
        pthread_rwlock_unlock(&rwlock_);
    }

    void Node::UnregisterPeer(const std::string& node_id){
        pthread_rwlock_trywrlock(&rwlock_);
        peers_.erase(node_id);
        pthread_rwlock_unlock(&rwlock_);
    }

    bool Node::HasPeer(const std::string& node_id){
        pthread_rwlock_tryrdlock(&rwlock_);
        bool found = peers_.find(node_id) != peers_.end();
        pthread_rwlock_unlock(&rwlock_);
        return found;
    }

    bool Node::ConnectTo(const NodeAddress &address){
        if(IsStarting() || IsSynchronizing()){
            WaitForState(State::kRunning);
            if(!IsRunning()) {
                LOG(WARNING) << "server is in " << GetState() << " state, not connecting!";
                return false;
            }
        }

        //-- Attention! --
        // this is not a memory leak, the memory will be freed upon
        // the session being closed
        return (new PeerSession(address))->Connect();
    }
}