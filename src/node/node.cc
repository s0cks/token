#include "node/node.h"

namespace Token{
    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        BlockChainServer* instance = BlockChainServer::GetInstance();
        ByteBuffer* bb = instance->GetReadBuffer();
        *buff = uv_buf_init((char*)bb->GetBytes(), suggested_size);
    }

    BlockChainServer* BlockChainServer::GetInstance(){
        static BlockChainServer instance;
        return &instance;
    }

    int BlockChainServer::Initialize(int port){
        BlockChainServer* instance = BlockChainServer::GetInstance();
        uv_loop_init(&instance->loop_);
        uv_tcp_init(&instance->loop_, &instance->server_);
        sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", port, &addr);
        std::cout << "Server listening @ " << port << std::endl;
        uv_tcp_bind(&instance->server_, (const struct sockaddr*)&addr, 0);
        int err = uv_listen((uv_stream_t*)&instance->server_, 100, [](uv_stream_t* server, int status){
            std::cout << "Connecting" << std::endl;
            BlockChainServer::GetInstance()->OnNewConnection(server, status);
        });
        return err;
    }

    void BlockChainServer::Start(){
        BlockChainServer* instance = BlockChainServer::GetInstance();
        instance->state_ = State::kRunning;
        uv_run(&instance->loop_, UV_RUN_DEFAULT);
    }

    bool BlockChainServer::ConnectTo(const std::string& address, int port){
        PeerSession* peer = new PeerSession(&loop_, address, port);
        std::cout << "Connecting to " << address << ":" << port << std::endl;
        return peer->IsConnected();
    }

    void BlockChainServer::OnNewConnection(uv_stream_t *server, int status){
        std::cout << "New client" << std::endl;
        if(status == 0){
            ClientSession* session = new ClientSession(std::make_shared<uv_tcp_t>());
            std::cout << "new client connected!" << std::endl;
            uv_tcp_init(&loop_, session->GetConnection());
            if(uv_accept(server, (uv_stream_t*) session->GetConnection()) == 0){
                uv_stream_t* key = (uv_stream_t*)session->GetConnection();
                key->data = session;
                uv_read_start((uv_stream_t*)session->GetConnection(), AllocBuffer, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
                    BlockChainServer::GetInstance()->OnMessageRead(stream, nread, buff);
                });
                sessions_.insert({ key, session });
            } else{
                uv_read_stop((uv_stream_t*) session->GetConnection());
            }
        } else{
            std::cerr << "connection error: " << std::string(uv_strerror(status)) << std::endl;
        }
    }

    void BlockChainServer::Broadcast(uv_stream_t *from, Token::Message *msg){
        for(auto& it : sessions_){
            it.second->Send(msg);
        }
    }

    void BlockChainServer::Send(uv_stream_t* target, Message* msg){
        ByteBuffer bb;
        msg->Encode(&bb);

        uv_buf_t buff = uv_buf_init((char*)bb.GetBytes(), bb.WrittenBytes());
        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = buff.base;
        uv_write(req, target, &buff, 1, [](uv_write_t* req, int status){
            BlockChainServer::GetInstance()->OnMessageSent(req, status);
        });
    }

    void BlockChainServer::OnMessageSent(uv_write_t* req, int status){
        if(status != 0){
            std::cerr << "failed to send message: " << std::string(uv_strerror(status)) << std::endl;
            if(req) free(req);
        }
    }

    void BlockChainServer::RemoveClient(uv_stream_t *stream){

    }

    bool BlockChainServer::Handle(Token::Session *session, Token::Message *msg){
        return true;
    }

    void BlockChainServer::OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
        auto pos = sessions_.find(stream);
        std::cout << "Read message finding session" << std::endl;
        if(pos != sessions_.end()){
            if(nread == UV_EOF){
                RemoveClient(stream);
                std::cerr << "Disconnected" << std::endl;
                return;
            } else if(nread > 0){
                Session* session = pos->second;
                if(nread < 4096){
                    session->Append(buff);
                    Message* msg = session->GetNextMessage();
                    std::cout << "Session handling " << msg->GetName() << std::endl;
                    if(!session->Handle(msg)){
                        std::cerr << "server couldn't handle message" << std::endl;
                    }
                }
            } else if(nread < 0){
                std::cerr << "error: " << std::string(uv_strerror(nread)) << std::endl;
            } else if(nread == 0){
                std::cerr << "zero message size received" << std::endl;
            }
        } else{
            uv_read_stop(stream);
            std::cerr << "unrecognized client. disconnected" << std::endl;
        }
    }
}