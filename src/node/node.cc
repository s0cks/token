#include "node/node.h"
#include <iostream>
#include "message.h"

namespace Token{
    namespace Node{
        static void
        AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
            Server* instance = Server::GetInstance();
            ByteBuffer* read = instance->GetReadBuffer();
            *buff = uv_buf_init((char*)read->GetBytes(), suggested_size);
        }

        Server* Server::GetInstance(){
            static Server instance;
            return &instance;
        }

        void Server::AddPeer(const std::string &addr, int port){
            Server* instance = GetInstance();
            PeerSession* peer = new PeerSession(&instance->loop_, addr, port);
            instance->sessions_.insert({ peer->GetStream(), peer });
        }

        int Server::Initialize(int port, const std::string &paddr, int pport){
            Server* instance = GetInstance();
            instance->AddPeer(paddr, pport);
            return Initialize(port);
        }

        int Server::Initialize(int port){
            Server* instance = GetInstance();
            uv_loop_init(&instance->loop_);
            uv_tcp_init(&instance->loop_, &instance->server_);

            sockaddr_in addr;
            uv_ip4_addr("0.0.0.0", port, &addr);
            uv_tcp_bind(&instance->server_, (const struct sockaddr*)&addr, 0);
            int err = uv_listen((uv_stream_t*)&instance->server_,
                    100,
                    [](uv_stream_t* server, int status){
                Server::GetInstance()->OnNewConnection(server, status);
            });
            if(err == 0){
                std::cout << "Listening @ localhost:" << port << std::endl;
                uv_run(&instance->loop_, UV_RUN_DEFAULT);
                instance->running_ = true;
            }
            return err;
        }

        void Server::OnNewConnection(uv_stream_t *server, int status){
            std::cout << "Attempting connection..." << std::endl;
            if(status == 0){
                ClientSession* session = new ClientSession(std::make_shared<uv_tcp_t>());
                uv_tcp_init(&loop_, session->GetConnection());
                if(uv_accept(server, (uv_stream_t*)session->GetConnection()) == 0){
                    std::cout << "Accepted!" << std::endl;
                    uv_stream_t* key = (uv_stream_t*)session->GetConnection();
                    uv_read_start((uv_stream_t*) session->GetConnection(),
                            AllocBuffer,
                            [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
                        Server::GetInstance()->OnMessageReceived(stream, nread, buff);
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

        void Server::Broadcast(uv_stream_t* stream, Message* msg){
            for(auto& it : sessions_){
                if(it.second->IsPeerSession() && it.first != stream){
                    it.second->Send(msg);
                }
            }
        }

        void Server::Send(uv_stream_t* target, Message* msg){
            ByteBuffer bb;
            msg->Encode(&bb);

            uv_buf_t buff = uv_buf_init((char*) bb.GetBytes(), bb.WrittenBytes());
            uv_write_t* req = (uv_write_t*) malloc(sizeof(uv_write_t));
            req->data = buff.base;
            uv_write(req, target, &buff, 1, [](uv_write_t* req, int status){
                Server::GetInstance()->OnMessageSent(req, status);
            });
        }

        void Server::OnMessageSent(uv_write_t* req, int status){
            if(status != 0){
                std::cerr << "Failed to send message: " << std::string(uv_strerror(status)) << std::endl;
                if(req) free(req);
            }
        }

        bool Server::Handle(Session* session, Token::Message* msg){
            if(msg->IsRequest()){
                Request* request = msg->AsRequest();
                if(request->IsGetHeadRequest()){
                    Block* head = BlockChain::GetInstance()->GetHead();
                    GetHeadResponse response((*request->AsGetHeadRequest()), head);
                    session->Send(&response);
                    return true;
                }
            }
            return false;
        }

        void Server::OnMessageReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
            if(nread == UV_EOF){
                RemoveClient(stream);
                std::cout << "Disconnected" << std::endl;
            }

            auto pos = sessions_.find(stream);
            if(pos != sessions_.end()){
                if(nread == UV_EOF){
                    RemoveClient(stream);
                    std::cout << "Disconnected" << std::endl;
                } else if(nread > 0){
                    Session* session = pos->second;
                    if(nread < 4096){
                        session->Append(buff);
                        if(!Handle(session, session->GetNextMessage())){
                            std::cerr << "Server couldn't handle message from: " << session << std::endl;
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
    }
}