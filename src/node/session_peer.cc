#include "node/session.h"
#include "node/node.h"
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

    void PeerSession::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        PeerSession* session = (PeerSession*)handle->data;
        ByteBuffer* bb = session->GetReadBuffer();
        bb->clear();
        buff->base = (char*)bb->data();
        buff->len = bb->GetCapacity();
    }

    void* PeerSession::PeerSessionThread(void* data){
        PeerSession* session = (PeerSession*)data;
        NodeAddress address = session->node_addr_;

        uv_loop_t* loop = uv_loop_new();
        uv_tcp_init(loop, &session->socket_);
        uv_tcp_keepalive(&session->socket_, 1, 60);

        struct sockaddr_in addr;
        uv_ip4_addr(address.GetAddress().c_str(), address.GetPort(), &addr);

        int err;
        if((err = uv_tcp_connect(&session->conn_, &session->socket_, (const struct sockaddr*)&addr, &OnConnect)) != 0){
            LOG(ERROR) << "cannot connect to peer: " << uv_strerror(err);
            return nullptr; //TODO: fix error handling
        }

        uv_run(loop, UV_RUN_DEFAULT);
        return nullptr;
    }

    void PeerSession::OnConnect(uv_connect_t* conn, int status){
        PeerSession* session = (PeerSession*)conn->data;
        if(status != 0){
            LOG(ERROR) << "error connecting: " << uv_strerror(status);
            return;
        }

        std::string address = "127.0.0.1";
        uint32_t port = FLAGS_port;
        NodeInfo info(Node::GetInfo().GetNodeID(), address, port);
        session->Send(VersionMessage::NewInstance(info.GetNodeID()));

        if((status = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
            LOG(ERROR) << "client read error: " << uv_strerror(status);
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
            //TODO: wuut? delete task;
        }
    }

    void PeerSession::HandleGetBlocksMessage(HandleMessageTask* task){}

    void PeerSession::HandleVersionMessage(HandleMessageTask* task){
        VersionMessage* msg = (VersionMessage*)task->GetMessage();
        PeerSession* session = (PeerSession*)task->GetSession();

        NodeInfo ninfo = Node::GetInfo();
        session->Send(VerackMessage::NewInstance(NodeInfo(ninfo.GetNodeID(), "127.0.0.1", FLAGS_port)));

        uint32_t pheight = msg->GetHeight();
        BlockHeader head = BlockChain::GetHead();
        if(head.GetHeight() < pheight){
            Node::SetState(Node::kSynchronizing);
            session->Send(GetBlocksMessage::NewInstance());
        }
    }

    void PeerSession::HandleVerackMessage(HandleMessageTask* task){
        VerackMessage* msg = (VerackMessage*)task->GetMessage();
        PeerSession* session = (PeerSession*)task->GetSession();

        //TODO:
        // - nonce check
        // - state transition
        // - register peer

        LOG(INFO) << "registering peer: " << msg->GetID();
        Node::RegisterPeer(msg->GetID(), session);
    }

    void PeerSession::HandlePrepareMessage(HandleMessageTask* task){}

    void PeerSession::HandlePromiseMessage(HandleMessageTask* task){
        PromiseMessage* msg = (PromiseMessage*)task->GetMessage();
        PeerSession* session = (PeerSession*)task->GetSession();

        std::string node_id = msg->GetNodeID();
        uint32_t height = msg->GetHeight();
        if(!BlockMiner::VoteForProposal(node_id)){
            LOG(WARNING) << node_id << " couldn't vote for proposal #" << height;
            return;
        }

        LOG(INFO) << node_id << " voted for proposal #" << height << "!";
    }

    void PeerSession::HandleCommitMessage(HandleMessageTask* task){}

    void PeerSession::HandleAcceptedMessage(HandleMessageTask* task){
        AcceptedMessage* msg = (AcceptedMessage*)task->GetMessage();
        PeerSession* session = (PeerSession*)task->GetSession();

        std::string node_id = msg->GetNodeID();
        uint32_t height = msg->GetHeight();
        if(!BlockMiner::AcceptProposal(node_id)){
            LOG(WARNING) << node_id << " couldn't accept proposal #" << height;
            return;
        }

        LOG(INFO) << node_id << " accepted #" << height << "!";
    }

    void PeerSession::HandleRejectedMessage(HandleMessageTask* task){

    }

    //TODO: vectorize-responses
    void PeerSession::HandleGetDataMessage(HandleMessageTask* task){
        GetDataMessage* msg = (GetDataMessage*)task->GetMessage();
        PeerSession* session = (PeerSession*)task->GetSession();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "cannot get items from message";
            return;
        }

        for(auto& item : items){
            if(item.ItemExists()){
                uint256_t hash = item.GetHash();
                LOG(INFO) << "sending " << hash << "....";
                if(item.IsBlock()){
                    Block* block = nullptr;
                    if((block = BlockChain::GetBlockData(hash))){
                        LOG(INFO) << hash << " found in block chain!";
                        session->Send(BlockMessage::NewInstance(block));
                        return;
                    }

                    if((block = BlockPool::GetBlock(hash))){
                        LOG(INFO) << hash << " found in block pool!";
                        session->Send(BlockMessage::NewInstance(block));
                        return;
                    }
                } else if(item.IsTransaction()){
                    Transaction* tx;
                    if((tx = TransactionPool::GetTransaction(hash))){
                        LOG(INFO) << hash << " found in transaction pool!";
                        session->Send(TransactionMessage::NewInstance(tx));
                        return;
                    }
                }
            }
        }
    }

    void PeerSession::HandleBlockMessage(HandleMessageTask* task){
        PeerSession* session = (PeerSession*)task->GetSession();
        BlockMessage* msg = (BlockMessage*)task->GetMessage();

        Block* block = msg->GetBlock();
        uint256_t hash = block->GetHash();

        if(!BlockPool::AddBlock(block)){
            LOG(WARNING) << "couldn't add " << block->GetHeader() << " to block pool!";
            return;
        }

        LOG(INFO) << "downloaded block: " << block->GetHeader();
        session->OnHash(hash);
    }

    void PeerSession::HandleTransactionMessage(HandleMessageTask* task){

    }

    void PeerSession::HandleInventoryMessage(HandleMessageTask* task){
        PeerSession* session = (PeerSession*)task->GetSession();
        InventoryMessage* msg = (InventoryMessage*)task->GetMessage();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "couldn't get items from inventory";
            return;
        }

        LOG(INFO) << "received inventory of " << items.size() << " items, downloading...";
        if(!items.empty()) session->Send(GetDataMessage::NewInstance(items));
        if(Node::IsSynchronizing()) SCHEDULE(session->GetLoop(), SynchronizeBlocks, session, items);
    }

    void PeerSession::HandleSynchronizeBlocksTask(uv_work_t* handle){
        SynchronizeBlocksTask* task = (SynchronizeBlocksTask*)handle->data;
        PeerSession* session = (PeerSession*)task->GetSession();

        std::vector<InventoryItem> items;
        if(!task->GetItems(items)){
            LOG(WARNING) << "couldn't get items from task";
            return;
        }
        std::reverse(items.begin(), items.end());

        LOG(WARNING) << "waiting for " << items.size() << " items...";
        session->WaitForItems(items);
        LOG(WARNING) << "done waiting!";

        for(auto item = items.rbegin(); item != items.rend(); item++){
            LOG(INFO) << "appending: " << item->GetHash();
            Block* block = BlockPool::GetBlock(item->GetHash());
            if(!BlockChain::AppendBlock(block)){
                LOG(WARNING) << "couldn't append block: " << block->GetHeader();
                return;
            }
        }
    }

    void PeerSession::AfterSynchronizeBlocksTask(uv_work_t* handle, int status){
        Node::SetState(Node::kStarting);
        // if(handle->data) free(handle->data);
        // free(handle);
    }
}