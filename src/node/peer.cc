#include "node/peer.h"
#include "node/node.h"
#include "task.h"
#include "block_miner.h"

namespace Token{
    void PeerSession::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        buff->base = (char*)malloc(suggested_size);
        buff->len = suggested_size;
    }

    void PeerSession::Send(const Message& msg){
        LOG(INFO) << "sending " << msg;

        uint32_t type = static_cast<uint32_t>(msg.GetType());
        uint64_t size = msg.GetSize();
        uint64_t total_size = Message::kHeaderSize + size;

        uint8_t bytes[total_size];
        memcpy(&bytes[Message::kTypeOffset], &type, Message::kTypeLength);
        memcpy(&bytes[Message::kSizeOffset], &size, Message::kSizeLength);
        msg.Encode(&bytes[Message::kDataOffset], size);

        uv_buf_t buff = uv_buf_init((char*)bytes, total_size);
        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, GetStream(), &buff, 1, &OnMessageSent);
    }

    void PeerSession::OnMessageSent(uv_write_t *req, int status){
        if(status != 0) LOG(ERROR) << "failed to send message: " << uv_strerror(status);
        if(req){
            free(req);
        }
    }

    void* PeerSession::PeerSessionThread(void* data){
        PeerSession* session = (PeerSession*)data;
        NodeAddress address = session->node_addr_;
        LOG(INFO) << "connecting to peer " << address << "....";

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
        Node* node = Node::GetInstance();
        PeerSession* session = (PeerSession*)conn->data;
        if(status != 0){
            LOG(ERROR) << "error connecting: " << uv_strerror(status);
            return;
        }

        std::string address = "127.0.0.1";
        uint32_t port = FLAGS_port;
        NodeInfo info(node->GetInfo().GetNodeID(), address, port);
        session->Send(VersionMessage(info));

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

        Message* msg;
        if(!(msg = Message::Decode((uint8_t*)buff->base, buff->len))){
            LOG(INFO) << "couldn't decode message!";
            return;
        }

        LOG(INFO) << "received: " << (*msg);

        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = new HandleMessageTask(session, msg);
        switch(msg->GetType()){
#define DEFINE_HANDLER_CASE(Name) \
            case Message::Type::k##Name##Message: \
                uv_queue_work(stream->loop, work, &Handle##Name##Message, AfterHandleMessage); \
                break;
            FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE);
            case Message::Type::kUnknownMessage:
            default: //TODO: handle properly
                return;
        }
    }

    void PeerSession::HandleVersionMessage(uv_work_t* handle){
        Node* node = Node::GetInstance();
        HandleMessageTask* task = (HandleMessageTask*)handle->data;
        VersionMessage* msg = (VersionMessage*)task->GetMessage();
        PeerSession* session = (PeerSession*)task->GetSession();

        NodeInfo ninfo = node->GetInfo();
        session->Send(VerackMessage(NodeInfo(ninfo.GetNodeID(), "127.0.0.1", FLAGS_port)));
    }

    void PeerSession::HandleVerackMessage(uv_work_t* handle){
        Node* node = Node::GetInstance();
        HandleMessageTask* task = (HandleMessageTask*)handle->data;
        VerackMessage* msg = (VerackMessage*)task->GetMessage();
        PeerSession* session = (PeerSession*)task->GetSession();

        //TODO:
        // - nonce check
        // - state transition
        // - register peer

        LOG(INFO) << "registering peer: " << msg->GetID();
        Node::RegisterPeer(msg->GetID(), session);
    }

    void PeerSession::HandlePrepareMessage(uv_work_t* handle){}

    void PeerSession::HandlePromiseMessage(uv_work_t* handle){
        Node* node = Node::GetInstance();
        HandleMessageTask* task = (HandleMessageTask*)handle->data;
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

    void PeerSession::HandleCommitMessage(uv_work_t* handle){}

    void PeerSession::HandleAcceptedMessage(uv_work_t* handle){
        HandleMessageTask* task = (HandleMessageTask*)handle->data;
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

    void PeerSession::HandleRejectedMessage(uv_work_t* handle){

    }

    void PeerSession::HandleGetDataMessage(uv_work_t* handle){
        HandleMessageTask* task = (HandleMessageTask*)handle->data;
        GetDataMessage* msg = (GetDataMessage*)task->GetMessage();
        PeerSession* session = (PeerSession*)task->GetSession();

        uint256_t hash = HashFromHexString(msg->GetHash());

        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = new GetDataTask(session, hash);
        uv_queue_work(handle->loop, work, HandleGetDataTask, AfterHandleGetDataTask);
    }

    void PeerSession::HandleBlockMessage(uv_work_t* handle){

    }

    void PeerSession::HandleTransactionMessage(uv_work_t* handle){

    }

    void PeerSession::AfterHandleMessage(uv_work_t* handle, int status){
        delete (HandleMessageTask*)handle->data;
        free(handle);
    }

    void PeerSession::HandleGetDataTask(uv_work_t* handle){
        GetDataTask* task = (GetDataTask*)handle->data;
        PeerSession* session = (PeerSession*)task->GetSession();
        uint256_t hash = task->GetHash();

        Block* block = nullptr;
        if((block = BlockChain::GetBlockData(hash))){
            LOG(INFO) << hash << " found in block chain!";
            session->Send(BlockMessage(block));
            return;
        }

        if((block = BlockPool::GetBlock(hash))){
            LOG(INFO) << hash << " found in block pool!";
            session->Send(BlockMessage(block));
            return;
        }

        //TODO: get transactions

        // re-queue work
        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = task;
        uv_queue_work(handle->loop, work, HandleGetDataTask, AfterHandleGetDataTask);
    }

    void PeerSession::AfterHandleGetDataTask(uv_work_t* handle, int status){
        free(handle->data);
        free(handle);
    }
}