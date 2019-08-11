#include "node/node.h"
#include "blockchain.h"

namespace Token{
    struct SendMessageData{
        PeerSession* session;
        Message* message;
    };

    struct ShutdownMessageData{
        uv_loop_t* loop;
    };

    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        BlockChainServer* server = BlockChain::GetServerInstance();
        ByteBuffer* bb = server->GetReadBuffer();
        *buff = uv_buf_init((char*)bb->GetBytes(), bb->Size());
    }

    void BlockChainServer::AddPeer(const std::string &addr, int port){
        PeerSession* peer = new PeerSession(this, addr, port);
        if(!peer->Connect()){
            std::cerr << "Couldn't connect to peer" << std::endl;
        }
    }

    void BlockChainServer::OnNewConnection(uv_stream_t* server, int status){
        std::cout << "Attempting connection..." << std::endl;
        if(status == 0){
            ClientSession* session = new ClientSession(std::make_shared<uv_tcp_t>());
            uv_tcp_init(loop_, session->GetConnection());
            if(uv_accept(server, (uv_stream_t*)session->GetConnection()) == 0){
                std::cout << "Accepted!" << std::endl;
                uv_stream_t* key = (uv_stream_t*)session->GetConnection();
                uv_read_start((uv_stream_t*) session->GetConnection(),
                              AllocBuffer,
                              [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
                                  BlockChain::GetServerInstance()->OnMessageReceived(stream, nread, buff);
                              });
                sessions_.insert({ key, session });
            } else{
                std::cerr << "Error accepting connection: " << std::string(uv_strerror(status));
                uv_read_stop((uv_stream_t*) session->GetConnection());
            }
        } else{
            std::cerr << "Connection error: " << std::string(uv_strerror(status));
        }
    }

    void BlockChainServer::BroadcastToPeers(uv_stream_t *from, Token::Message *msg){
        for(auto& it : sessions_){
            if(it.second->IsPeerSession() && it.first != from){
                it.second->Send(it.first, msg);
            }
        }
    }

    void BlockChainServer::Broadcast(uv_stream_t* stream, Message* msg){
        std::cout << "Broadcasting to " << sessions_.size() << " peers" << std::endl;
        for(auto& it : sessions_){
            if(it.second->IsPeerSession()){
                std::cout << "Broadcasting to peer" << std::endl;
                Send(it.first, msg);
            }
        }
    }

    void BlockChainServer::Send(uv_stream_t* target, Message* msg){
        auto pos = sessions_.find(target);
        if(pos != sessions_.end()){
            std::cout << "Sending" << std::endl;
            PeerSession* peer = (PeerSession*) pos->second;
            peer->Send(target, msg);
        }
    }

    void BlockChainServer::Shutdown(){
        shutdown_handle_.data = loop_;
        uv_async_send(&shutdown_handle_);
    }

    void BlockChainServer::OnMessageSent(uv_write_t* req, int status){
        if(status != 0){
            std::cerr << "Failed to send message: " << std::string(uv_strerror(status)) << std::endl;
            if(req) free(req);
        }
    }

    void BlockChainServer::OnMessageReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
        auto pos = sessions_.find(stream);
        if(pos != sessions_.end()){
            if(nread == UV_EOF){
                std::cout << "Disconnected" << std::endl;
            } else if(nread > 0){
                Session* session = pos->second;
                if(nread < 4096){
                    session->Append(buff, nread);
                    std::cout << "Read: " << nread << std::endl;

                    Message msg;
                    if(!msg.Decode(session->GetReadBuffer())){
                        std::cerr << "Couldn't get next message" << std::endl;
                        uv_read_stop(stream);
                        return;
                    }
                    session->ClearReadBuffer();
                    if(!session->Handle(stream, &msg)){
                        std::cerr << "BlockChainServer couldn't handle message from: " << session << std::endl;
                        uv_read_stop(stream);
                        return;
                    }
                }
            } else if(nread < 0){
                std::cerr << "Error: " << std::to_string(nread) << std::endl;
            } else if(nread == 0){
                std::cerr << "Zero message size received" << std::endl;
            }
        } else{
            uv_read_stop(stream);
            std::cerr << "Unrecognized client. Disconnecting" << std::endl;
        }
    }

    static void send_cb(uv_async_t* handle){
        SendMessageData* data = (SendMessageData*) handle->data;
        data->session->Send(data->session->GetStream(), data->message);
    }

    static void shutdown_cb(uv_async_t* handle){
        ShutdownMessageData* data = (ShutdownMessageData*) handle->data;
        uv_stop(data->loop);
    }

    BlockChainServer::BlockChainServer():
        thread_(),
        port_(-1),
        loop_(uv_loop_new()),
        server_(),
        sessions_(),
        running_(false),
        shutdown_handle_(),
        send_handle_(),
        read_buff_(MAX_BUFF_SIZE){
        uv_async_init(loop_, &shutdown_handle_, &shutdown_cb);
        uv_async_init(loop_, &send_handle_, &send_cb);
    }

    int BlockChainServer::Start(){
        std::cout << "Starting server on localhost:" << GetPort() << std::endl;
        sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", GetPort(), &addr);
        uv_tcp_init(loop_, &server_);
        uv_tcp_bind(&server_, (const struct sockaddr*)&addr, 0);
        int err = uv_listen((uv_stream_t*)&server_,
                            100,
                            [](uv_stream_t* server, int status){
                                BlockChain::GetServerInstance()->OnNewConnection(server, status);
                            });
        if(err == 0){
            std::cout << "Listening @ localhost:" << GetPort() << std::endl;
            running_ = true;
            uv_run(loop_, UV_RUN_DEFAULT);
        }
        return err;
    }
}