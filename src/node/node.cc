#include <random>
#include "node/node.h"
#include "node/message.h"
#include "task.h"

namespace Token{
#define SCHEDULE(Loop, Name, ...) \
    uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));\
    work->data = new Name##Task(__VA_ARGS__); \
    uv_queue_work((Loop), work, Handle##Name##Task, After##Name##Task);

    /*
     * void BlockChainServer::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        BlockChainServer* instance = GetInstance();
        buff->base = (char*)malloc(suggested_size);
        buff->len = suggested_size;
    }
     */

    void Node::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        buff->base = (char*)malloc(suggested_size);
        buff->len = suggested_size;
    }

    void* Node::NodeThread(void* ptr){
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

        NodeInfo info = node->GetInfo();
        LOG(INFO) << "[" << info.GetNodeID() << "] server listening: @" << FLAGS_port;
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(0);
    }

    void Node::OnNewConnection(uv_stream_t* stream, int status){
        LOG(INFO) << "client is connecting...";
        if(status != 0){
            LOG(ERROR) << "connection error: " << uv_strerror(status);
            return;
        }

        Node* node = (Node*)stream->data;
        NodeSession* session = new NodeSession();
        uv_tcp_init(stream->loop, (uv_tcp_t*)session->GetHandle());

        if((status = uv_accept(stream, (uv_stream_t*)session->GetHandle())) != 0){
            LOG(ERROR) << "client accept error: " << uv_strerror(status);
            return;
        }

        if((status = uv_read_start(session->GetHandle(), &AllocBuffer, &OnMessageReceived)) != 0){
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

    void Node::HandleGetDataMessage(uv_work_t* handle){
        //TODO: implement
    }

    void Node::HandleTransactionMessage(uv_work_t* handle){
        //TODO: implement
    }

    void Node::HandleVersionMessage(uv_work_t* handle){
        Node* node = GetInstance();
        HandleMessageTask* task = (HandleMessageTask*)handle->data;
        NodeSession* session = (NodeSession*)task->GetSession();
        //TODO:
        // - state check
        // - version check
        // - echo nonce
        NodeInfo ninfo = node->GetInfo();
        session->Send(VersionMessage(NodeInfo(ninfo.GetNodeID(), "127.0.0.1", FLAGS_port)));
    }

    void Node::HandleVerackMessage(uv_work_t* handle){
        Node* node = GetInstance();
        HandleMessageTask* task = (HandleMessageTask*)handle->data;
        VerackMessage* msg = (VerackMessage*)task->GetMessage();
        NodeSession* session = (NodeSession*)task->GetSession();

        NodeAddress paddr = msg->GetCallbackAddress();

        //TODO:
        // - verify nonce
        LOG(INFO) << "client connected!";
        session->Send(VerackMessage(node->GetInfo()));
        session->SetState(NodeSession::kConnected);
        if(!HasPeer(msg->GetID())){
            LOG(WARNING) << "connecting to peer: " << paddr << "....";
            if(!Node::ConnectTo(paddr)){
                LOG(WARNING) << "couldn't connect to peer: " << paddr;
            }
        }
    }

    void Node::HandlePrepareMessage(uv_work_t* handle){
        Node* node = Node::GetInstance();
        HandleMessageTask* task = (HandleMessageTask*)handle->data;
        NodeSession* session = (NodeSession*)task->GetSession();
        PrepareMessage* msg = (PrepareMessage*)task->GetMessage();

        Proposal proposal(msg->GetNodeID(), msg->GetHeight(), msg->GetHash());
        if(!proposal.IsValid()){
            LOG(WARNING) << "proposal #" << proposal.GetHeight() << " from " << proposal.GetProposer() << " is invalid!";
            return;
        }

        uint256_t hash = proposal.GetHash();
        session->Send(GetDataMessage(hash));
        session->WaitForHash(hash);
        session->Send(PromiseMessage(node->GetInfo(), proposal));
    }

    void Node::HandlePromiseMessage(uv_work_t* handle){}

    void Node::HandleCommitMessage(uv_work_t* handle){
        NodeInfo info = Node::GetInfo();
        HandleMessageTask* task = (HandleMessageTask*)handle->data;
        NodeSession* session = (NodeSession*)task->GetSession();
        CommitMessage* msg = (CommitMessage*)task->GetMessage();

        uint32_t height = msg->GetHeight();
        uint256_t hash = msg->GetHash();

        Block* block;
        if(!(block = BlockPool::GetBlock(hash))){
            LOG(WARNING) << "couldn't find block " << hash << " from pool!";
            goto exit;
        }

        if(!BlockChain::AppendBlock(block)){
            LOG(WARNING) << "couldn't append block: " << hash;
            goto exit;
        }

        session->Send(AcceptedMessage(info, height, HexString(hash)));
    exit:
        if(block) delete block;
        return;
    }

    void Node::HandleBlockMessage(uv_work_t* handle){
        HandleMessageTask* task = (HandleMessageTask*)handle->data;
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

    void Node::HandleAcceptedMessage(uv_work_t* handle){

    }

    void Node::HandleRejectedMessage(uv_work_t* handle){

    }

    void Node::AfterHandleMessage(uv_work_t* handle, int status){
        delete (HandleMessageTask*)handle->data;
        free(handle);
    }

    bool Node::Broadcast(const Message& msg){
        Node* node = GetInstance();
        pthread_rwlock_tryrdlock(&node->rwlock_);
        for(auto& it : node->peers_){
            it.second->Send(msg);
        }
        pthread_rwlock_unlock(&node->rwlock_);
        return true;
    }
}