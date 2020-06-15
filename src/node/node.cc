#include <algorithm>
#include <random>
#include "node/node.h"
#include "node/message.h"
#include "node/task.h"
#include "block_miner.h"

namespace Token{
#define SCHEDULE(Loop, Name, ...)({ \
    uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));\
    work->data = new Name##Task(__VA_ARGS__); \
    uv_queue_work((Loop), work, Handle##Name##Task, After##Name##Task); \
})
#define RESCHEDULE(Loop, Name, TaskInstance)({\
    uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t)); \
    work->data = (TaskInstance); \
    uv_queue_work((Loop), work, Handle##Name##Task, After##Name##Task); \
})

    Node* Node::GetInstance(){
        static Node instance;
        return &instance;
    }

    NodeInfo Node::GetInfo(){
        Node* node = GetInstance();
        pthread_rwlock_tryrdlock(&node->rwlock_);
        NodeInfo info = node->info_;
        pthread_rwlock_unlock(&node->rwlock_);
        return info;
    }

    void Node::SetState(Node::State state){
        Node* node = GetInstance();
        pthread_mutex_trylock(&node->smutex_);
        node->state_ = state;
        pthread_cond_signal(&node->scond_);
        pthread_mutex_unlock(&node->smutex_);
    }

    void Node::WaitForState(Node::State state){
        Node* node = GetInstance();
        pthread_mutex_trylock(&node->smutex_);
        while(node->state_ != state) pthread_cond_wait(&node->scond_, &node->smutex_);
        pthread_mutex_unlock(&node->smutex_);
        return;
    }

    Node::State Node::GetState() {
        Node* node = GetInstance();
        pthread_mutex_trylock(&node->smutex_);
        Node::State state = node->state_;
        pthread_mutex_unlock(&node->smutex_);
        return state;
    }

    uint32_t Node::GetNumberOfPeers(){
        Node* node = GetInstance();
        pthread_rwlock_tryrdlock(&node->rwlock_);
        uint32_t peers = node->peers_.size();
        pthread_rwlock_unlock(&node->rwlock_);
        return peers;
    }

    void Node::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        NodeSession* session = (NodeSession*)handle->data;
        ByteBuffer* bb = session->GetReadBuffer();
        bb->clear();

        buff->base = (char*)bb->data();
        buff->len = bb->GetCapacity();
    }

    void* Node::NodeThread(void* ptr){
        SetState(State::kStarting);

        Node* node = Node::GetInstance();
        uv_loop_t* loop = uv_loop_new();

        struct sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", FLAGS_port, &addr);

        uv_tcp_init(loop, node->GetHandle());
        uv_tcp_keepalive(node->GetHandle(), 1, 60);
        int err;
        if((err = uv_tcp_bind(node->GetHandle(), (const struct sockaddr*)&addr, 0)) != 0){
            LOG(ERROR) << ""; //TODO:
            pthread_exit(0);
        }

        if((err = uv_listen((uv_stream_t*)node->GetHandle(), 100, &OnNewConnection)) != 0){
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

        if((status = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
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
        NodeInfo info = Node::GetInfo();
        session->Send(VersionMessage::NewInstance(info.GetNodeID()));
    }

    void Node::HandleVerackMessage(HandleMessageTask* task){
        VerackMessage* msg = (VerackMessage*)task->GetMessage();
        NodeSession* session = (NodeSession*)task->GetSession();

        NodeAddress paddr = msg->GetCallbackAddress();

        //TODO:
        // - verify nonce
        LOG(INFO) << "client connected!";
        session->Send(VerackMessage::NewInstance(Node::GetInfo()));
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
        session->Send(PromiseMessage::NewInstance(Node::GetInfo(), (*proposal)));
    }

    void Node::HandlePromiseMessage(HandleMessageTask* task){}

    void Node::HandleCommitMessage(HandleMessageTask* task){
        NodeInfo info = Node::GetInfo();
        NodeSession* session = (NodeSession*)task->GetSession();
        CommitMessage* msg = (CommitMessage*)task->GetMessage();

        uint32_t height = msg->GetHeight();
        uint256_t hash = msg->GetHash();

        Block* block;
        if(!(block = BlockPool::GetBlock(hash))){
            LOG(WARNING) << "couldn't find block " << hash << " from pool waiting....";
            goto exit;
        }

        if(!BlockChain::AppendBlock(block)){
            LOG(WARNING) << "couldn't append block: " << hash;
            goto exit;
        }

        if(!BlockPool::RemoveBlock(hash)){
            LOG(WARNING) << "couldn't remove block from pool: " << hash;
            goto exit;
        }

        session->Send(AcceptedMessage::NewInstance(info, Proposal(msg->GetNodeID(), height, hash)));
    exit:
        return;
    }

    void Node::HandleBlockMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        BlockMessage* msg = (BlockMessage*)task->GetMessage();

        Block* block = msg->GetBlock();
        uint256_t hash = block->GetHash();

        if(!BlockPool::PutBlock(block)){
            LOG(WARNING) << "couldn't add block to pool: " << hash;
            return;
        }

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

        if(!items.empty()) session->Send(GetDataMessage::NewInstance(items));
    }

    void Node::HandleGetBlocksMessage(HandleMessageTask* task){
        NodeSession* session = (NodeSession*)task->GetSession();
        GetBlocksMessage* msg = (GetBlocksMessage*)task->GetMessage();

        uint256_t start = msg->GetHeadHash();
        uint256_t stop = msg->GetStopHash();

        std::vector<InventoryItem> items;
        if(stop.IsNull()){
            uint32_t amt = std::min(GetBlocksMessage::kMaxNumberOfBlocks, BlockChain::GetHeight());
            LOG(INFO) << "sending " << amt << " blocks...";

            BlockHeader start_block = BlockChain::GetBlock(start);
            BlockHeader stop_block = BlockChain::GetBlock(start_block.GetHeight() > amt ? start_block.GetHeight() + amt : amt);

            for(uint32_t idx = start_block.GetHeight();
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
        Node* node = GetInstance();
        pthread_rwlock_tryrdlock(&node->rwlock_);
        for(auto& it : node->peers_){
            it.second->Send(msg);
        }
        pthread_rwlock_unlock(&node->rwlock_);
        return true;
    }

    void Node::RegisterPeer(const std::string& node_id, PeerSession* peer){
        Node* node = GetInstance();
        pthread_rwlock_trywrlock(&node->rwlock_);
        node->peers_.insert({ node_id, peer });
        pthread_rwlock_unlock(&node->rwlock_);
    }

    void Node::UnregisterPeer(const std::string& node_id){
        Node* node = GetInstance();
        pthread_rwlock_trywrlock(&node->rwlock_);
        node->peers_.erase(node_id);
        pthread_rwlock_unlock(&node->rwlock_);
    }

    bool Node::HasPeer(const std::string& node_id){
        Node* node = GetInstance();
        pthread_rwlock_tryrdlock(&node->rwlock_);
        bool found = node->peers_.find(node_id) != node->peers_.end();
        pthread_rwlock_unlock(&node->rwlock_);
        return found;
    }

    bool Node::ConnectTo(const NodeAddress &address){
        Node* node = GetInstance();
        if(IsStarting() || IsSynchronizing()){
            pthread_mutex_trylock(&node->smutex_);
            while(node->state_ != kRunning) pthread_cond_wait(&node->scond_, &node->smutex_);
            pthread_mutex_unlock(&node->smutex_);
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