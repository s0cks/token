#include <algorithm>
#include <random>
#include <condition_variable>

#include "alloc/scope.h"
#include "node/node.h"
#include "node/message.h"
#include "node/task.h"
#include "block_miner.h"
#include "configuration.h"

namespace Token{
//TODO: remove:
#define SCHEDULE(Loop, Name, ...)({ \
    uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));\
    work->data = new Name##Task(__VA_ARGS__); \
    uv_queue_work((Loop), work, Handle##Name##Task, After##Name##Task); \
})

    static pthread_t thread_ = 0;
    static uv_tcp_t handle_;
    static uv_async_t aterm_;

    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static Node::State state_ = Node::State::kStopped;
    static std::string node_id_;
    static std::map<std::string, PeerSession*> peers_ = std::map<std::string, PeerSession*>();

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    void Node::LoadNodeInformation(){
        LOCK_GUARD;
        libconfig::Setting& node = BlockChainConfiguration::GetProperty("Node", libconfig::Setting::TypeGroup);
        if(!node.exists("id")){
            node.add("id", libconfig::Setting::TypeString) = GenerateNonce();
            BlockChainConfiguration::SaveConfiguration();
        }

        std::string node_id;
        node.lookupValue("id", node_id);
        node_id_ = node_id;
    }

    void Node::LoadPeers(){
        LOCK_GUARD;
        libconfig::Setting& peers = BlockChainConfiguration::GetProperty("Peers", libconfig::Setting::TypeArray);
        auto iter = peers.begin();
        while(iter != peers.end()){
            NodeAddress address((*iter));
            if(!ConnectTo(address)) LOG(WARNING) << "couldn't connect to peer: " << address;
            iter++;
        }
    }

    void Node::SavePeers(){
        LOCK_GUARD;
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
        LOCK_GUARD;
        return node_id_;
    }

    void Node::Start(){
        if(!IsStopped()) return;
        LoadNodeInformation();
        LoadPeers();
        if(pthread_create(&thread_, NULL, &NodeThread, NULL) != 0){
            std::stringstream ss;
            ss << "Couldn't start block chain server thread";
            CrashReport::GenerateAndExit(ss);
        }
    }

    bool Node::Shutdown(){
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

    bool Node::WaitForShutdown(){
        WaitForState(State::kStopped);
        return true;
    }

    void Node::SetState(Node::State state){
        LOCK;
        state_ = state;
        SIGNAL_ALL;
    }

    void Node::WaitForState(Node::State state){
        LOCK;
        while(state_ != state) WAIT;
    }

    Node::State Node::GetState() {
        LOCK_GUARD;
        return state_;
    }

    uint32_t Node::GetNumberOfPeers(){
        LOCK_GUARD;
        uint32_t peers = peers_.size();
        return peers;
    }

    void Node::HandleTerminateCallback(uv_async_t* handle){
        uv_stop(handle->loop);
    }

    void* Node::NodeThread(void* ptr){
#ifdef TOKEN_DEBUG
        LOG(INFO) << "starting server thread...";
#endif//TOKEN_DEBUG

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

#if defined(TOKEN_DEBUG)||defined(TOKEN_VERBOSE)
        LOG(INFO) << "node " << GetNodeID() << " listening @" << FLAGS_port;
#else
        LOG(INFO) << "server listening @" << FLAGS_port;
#endif//TOKEN_DEBUG or TOKEN_VERBOSE
        SetState(State::kRunning);
        uv_run(loop, UV_RUN_DEFAULT);

        uv_loop_close(loop);
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
        Scope scope;
        std::vector<Message*> messages;
        do{
            uint32_t mtype = 0;
            memcpy(&mtype, &buff->base[offset + Message::kTypeOffset], Message::kTypeLength);

            uint64_t msize = 0;
            memcpy(&msize, &buff->base[offset + Message::kSizeOffset], Message::kSizeLength);

            Message* msg = nullptr;
            if(!(msg = Message::Decode(static_cast<Message::MessageType>(mtype), msize, (uint8_t*)&buff->base[offset + Message::kDataOffset]))){
                std::stringstream ss;
                ss << "Couldn't decode message";
                ss << "\t - Type: " << mtype;
                ss << "\t - Size: " << msize;
                ss << "\t - Offset: " << offset;
                CrashReport::GenerateAndExit(ss);
            }

            scope.Retain(msg);
            messages.push_back(msg);
            offset += (msize + Message::kHeaderSize);
        } while((offset + Message::kHeaderSize) < nread);

        for(size_t idx = 0; idx < messages.size(); idx++){
            Message* msg = messages[idx];
            HandleMessageTask* task = new HandleMessageTask(session, msg);//TODO: implement better memory management
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

        Scope scope;
        std::vector<Message*> response;
        for(auto& item : items){
            uint256_t hash = item.GetHash();
            if(item.ItemExists()){
                if(item.IsBlock()){
                    Block* block = nullptr;
                    if(BlockChain::HasBlock(hash)){
                        block = BlockChain::GetBlockData(hash);
                    } else if(BlockPool::HasBlock(hash)){
                        block = BlockPool::GetBlock(hash);
                    } else{
                        //TODO: return 404
#ifdef TOKEN_DEBUG
                        LOG(WARNING) << "cannot find block: " << hash;
#endif//TOKEN_DEBUG
                        return;
                    }

                    Message* data = BlockMessage::NewInstance(block);
                    scope.Retain(data);
                    response.push_back(data);
                } else if(item.IsTransaction()){
                    Transaction* tx = nullptr;
                    if(TransactionPool::HasTransaction(hash)){
                        tx = TransactionPool::GetTransaction(hash);
                    } else{
                        //TODO: return 404
#ifdef TOKEN_DEBUG
                        LOG(WARNING) << "couldn't find transaction: " << hash;
#endif//TOKEN_DEBUG
                        return;
                    }

                    Message* data = TransactionMessage::NewInstance(tx);
                    scope.Retain(data);
                    response.push_back(data);
                }
            } else{
                //TODO: return 500
#ifdef TOKEN_DEBUG
                LOG(WARNING) << "item is invalid: " << item;
#endif//TOKEN_DEBUG
            }
        }

        session->Send(response);
    }

    void Node::HandleVersionMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        //TODO:
        // - state check
        // - version check
        // - echo nonce
        session->Send(VersionMessage::NewInstance(GetNodeID()));
    }

    //TODO:
    // - verify nonce
    void Node::HandleVerackMessage(HandleMessageTask* task){
        VerackMessage* msg = (VerackMessage*)task->GetMessage();
        NodeSession* session = (NodeSession*)task->GetSession();
        NodeAddress paddr = msg->GetCallbackAddress();

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
        //TODO: session->OnHash(hash); - move to block chain class, block calling thread
    }

    void Node::HandleTransactionMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        TransactionMessage* msg = (TransactionMessage*)task->GetMessage();

        Transaction* tx = msg->GetTransaction();
        TransactionPool::PutTransaction(tx);
        //TODO: session->OnHash(tx->GetHash());
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
        LOCK_GUARD;
        for(auto& it : peers_) it.second->Send(msg);
        return true;
    }

    void Node::RegisterPeer(const std::string& node_id, PeerSession* peer){
        LOCK_GUARD;
        peers_.insert({ node_id, peer });
        SavePeers();
    }

    void Node::UnregisterPeer(const std::string& node_id){
        LOCK_GUARD;
        peers_.erase(node_id);
    }

    bool Node::HasPeer(const std::string& node_id){
        LOCK_GUARD;
        return peers_.find(node_id) != peers_.end();
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