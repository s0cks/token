#include <glog/logging.h>
#include "server.h"
#include "peer.h"

namespace Token{
    BlockChainServer::BlockChainServer():
        loop_(uv_loop_new()),
        server_(),
        thread_(),
        broadcast_(),
        sessions_(){
        uv_loop_init(loop_);
        uv_async_init(loop_, &broadcast_, OnBroadcast);
    }

    BlockChainServer::~BlockChainServer(){

    }

    BlockChainServer* BlockChainServer::GetInstance(){
        static BlockChainServer instance;
        return &instance;
    }

    ByteBuffer* BlockChainServer::GetReadBuffer(){
        static ByteBuffer instance;
        return &instance;
    }

    void BlockChainServer::AllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buff){
        buff->base = (char*)malloc(suggested_size);
        buff->len = suggested_size;
    }

    void BlockChainServer::OnRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
        BlockChainServer* instance = GetInstance();

        auto pos = instance->sessions_.find(stream);
        if(pos == instance->sessions_.end()){
            return;
        }

        PeerSession* session = (PeerSession*)pos->second;
        if(nread > 0){
            LOG(INFO) << "read " << nread << "bytes from client";

            Message msg;
            if(!msg.Decode((uint8_t*)buff->base, nread)){
                LOG(ERROR) << "couldn't decode message from client";
                Disconnect(session);
                return;
            }

            if(!session->Handle(stream, &msg)){
                LOG(ERROR) << "couldn't handle message from session";
                Disconnect(session);
            }
        } else if(nread < 0){
            LOG(ERROR) << "error reading from client: " << std::string(uv_strerror(nread));
            Disconnect(session);
        } else if(nread == 0){
            LOG(WARNING) << "zero message size received from client";
        } else if(nread == UV_EOF){
            LOG(INFO) << "end-of-stream reached from client, disconnecting...";
            Disconnect(session);
        }
    }

    void BlockChainServer::Disconnect(Token::PeerSession* session){

    }

    void BlockChainServer::Register(Token::PeerSession* session){
        GetInstance()->sessions_.insert({ (uv_stream_t*)session->GetConnection(), session });
    }

    void BlockChainServer::OnNewConnection(uv_stream_t* stream, int status){
        LOG(INFO) << "client is connecting...";
        if(status != 0){
            LOG(ERROR) << "connection error: " << std::string(uv_strerror(status));
            return;
        }

        BlockChainServer* instance = (BlockChainServer*) stream->data;
        std::shared_ptr<uv_tcp_t> key = std::make_shared<uv_tcp_t>();
        PeerSession* client = new PeerSession(instance->loop_, key);
        uv_tcp_init(instance->loop_, key.get());

        int rc;
        if((rc = uv_accept(stream, (uv_stream_t*)key.get())) == 0){
            LOG(INFO) << "client accepted!";
            LOG(WARNING) << "performing handshake....";

            if(!client->PerformHandshake()){
                LOG(ERROR) << "handshake failed!";
                Disconnect(client);
                return;
            }

            instance->sessions_.insert({ (uv_stream_t*)key.get(), client });
            if((rc = uv_read_start((uv_stream_t*)key.get(), AllocBuffer, OnRead)) < 0){
                LOG(ERROR) << "error reading from client: " << std::string(uv_strerror(rc));
                return;
            }
            return;
        }
        LOG(ERROR) << "error accepting new client: " << std::string(uv_strerror(rc));
        LOG(ERROR) << "disconnecting....";
        Disconnect(client);
    }

    int BlockChainServer::Start(int port){
        LOG(INFO) << "starting BlockChainServer....";
        uv_tcp_init(loop_, &server_);
        server_.data = this;
        sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", port, &addr);
        uv_tcp_bind(&server_, (const struct sockaddr*)&addr, 0);
        int rc;
        if((rc = uv_listen((uv_stream_t *) &server_, 100, OnNewConnection)) == 0){
            LOG(INFO) << "BlockChainServer listening @ localhost:" << port;
            uv_run(loop_, UV_RUN_DEFAULT);
        }
        return rc;
    }

    struct StartCommand{
        int port;
        BlockChainServer* instance;
    };

    void* BlockChainServer::ServerThread(void* data){
        StartCommand* cmd = (StartCommand*) data;
        BlockChainServer* instance = cmd->instance;
        char* result;
        int rc;
        if((rc = instance->Start(cmd->port)) < 0){
            const char* err_msg = uv_strerror(rc);
            result = (char*)malloc(sizeof(err_msg));
            strcpy(result, err_msg);
        } else{
            result = (char*)"done";
        }
        pthread_exit(result);
    }

    int BlockChainServer::AddPeer(const std::string &address, int port){
        return (new PeerClient(address, port))->Connect();
    }

    int BlockChainServer::Initialize(int port){
        StartCommand* cmd = new StartCommand();
        cmd->port = port;
        cmd->instance = GetInstance();
        pthread_create(&GetInstance()->thread_, NULL, ServerThread, cmd);
        return true;
    }

    int BlockChainServer::ShutdownAndWait(){
        void* result;
        if(pthread_join(GetInstance()->thread_, &result) != 0){
            LOG(ERROR) << "unable to join server thread";
            return false;
        }
        LOG(INFO) << "server shutdown with result: " << (char*)result;
        return true;
    }

    void BlockChainServer::OnBroadcast(uv_async_t *handle){
        Message* msg = (Message*) handle->data;
        Message* dup = new Message(*msg);

        LOG(INFO) << "broadcasting " << dup->ToString() << " to " << GetInstance()->sessions_.size() << " peers";
        int idx = 0;
        for(auto& it : GetInstance()->sessions_){
            if(!it.second->Send(dup)){
                LOG(ERROR) << "couldn't send to peer " << idx;
            }
            idx++;
        }

        delete dup;
    }

    bool BlockChainServer::Broadcast(uv_stream_t *stream, Token::Message *msg){
        GetInstance()->broadcast_.data = msg;
        uv_async_send(&GetInstance()->broadcast_);
        return true;
    }
}