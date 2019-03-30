#include "client.h"
#include <iostream>

namespace Token{
    namespace Client{
        static void
        AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
            ByteBuffer* read = Session::GetInstance()->GetReadBuffer();
            *buff = uv_buf_init((char*) read->GetBytes(), suggested_size);
        }

        Message* Session::GetMessage(){
            Message* msg = nullptr;
            uint32_t type = GetReadBuffer()->GetInt();
            msg = Message::Decode(type, GetReadBuffer());
            GetReadBuffer()->Clear();
            return msg;
        }

        void Session::OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
            if(nread == UV_EOF){
                std::cerr << "Disconnected" << std::endl;
                uv_read_stop(stream);
            } else if(nread > 0){
                GetReadBuffer()->PutBytes((uint8_t*) buff->base, buff->len);
                Message* msg = GetMessage();
                if(msg->IsBlock()){
                    Block* block = msg->AsBlock()->GetBlock();
                    std::cout << (*block) << std::endl;
                }
            } else if(nread == 0){
                std::cout << "Read 0" << std::endl;
                GetReadBuffer()->Clear();
            } else{
                std::cerr << "Error reading" << std::endl;
                uv_read_stop(stream);
            }
        }

        Session* Session::GetInstance(){
            static Session instance;
            return &instance;
        }

        void Session::Initialize(int port, const std::string &addr){
            Session* instance = GetInstance();
            uv_tcp_init(&instance->loop_, &instance->stream_);
            uv_ip4_addr(addr.c_str(), port, &instance->addr_);
            uv_tcp_connect(&instance->conn_, &instance->stream_, (const struct sockaddr*)&instance->addr_, [](uv_connect_t* conn, int status){
                Session::GetInstance()->OnConnect(conn, status);
            });
            if(!instance->IsRunning()){
                uv_run(&instance->loop_, UV_RUN_DEFAULT);
                instance->running_ = true;
            }
        }

        void Session::OnMessageSend(uv_write_t* req, int status){
            if(status == 0){
                if(req) free(req);
            } else{
                std::cerr << "Error writing message" << std::endl;
            }
        }

        void Session::ReadInput(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
            if(nread < 0){
                std::cout << "Stopping..." << std::endl;
                uv_read_stop(stream);
            } else{
                std::string msg(buff->base, nread - 1);
                if(msg.compare(0, 8, "getblock") == 0){
                    std::string hash = msg.erase(0, 9);
                    GetBlockMessage m(hash);
                    Send(&m);
                } else if(msg.compare(0, 6, "create") == 0){
                    std::string height = msg.substr(msg.length() - 1);
                    std::cout << "height: " << height << std::endl;
                    std::string phash = msg.substr(7, 64);
                    std::cout << "Previous Hash: " << phash << std::endl;
                    Block* blk = new Block(phash, static_cast<uint32_t>(atoi(height.c_str())));
                    Transaction* tx = blk->CreateTransaction();
                    tx->AddInput(blk->GetPreviousHash(), 0);
                    tx->AddOutput("TestUser", "TestToken");
                    AppendBlockMessage m(blk);
                    Send(&m);
                } else if(msg.compare(0, 7, "gethead") == 0){
                    GetHeadMessage m;
                    Send(&m);
                } else if(msg.compare(0, 7, "connect") == 0){
                    std::string ports = msg.erase(0, 8);
                    int port = atoi(ports.c_str());
                    ConnectMessage m("localhost", port);
                    Send(&m);
                }
            }
        }

        void Session::Send(Token::Message* msg){
            std::cout << "Sending " << msg->GetName() << std::endl;
            ByteBuffer bb;
            msg->Encode(&bb);
            uv_buf_t buff = uv_buf_init((char*) bb.GetBytes(), bb.WrittenBytes());
            uv_write_t* req = (uv_write_t*) malloc(sizeof(uv_write_t));
            req->data = buff.base;
            uv_write(req, handle_, &buff, 1, [](uv_write_t* req, int status){
                Session::GetInstance()->OnMessageSend(req, status);
            });
        }

        void Session::OnConnect(uv_connect_t *connection, int status){
            if(status == 0){
                std::cout << "Connected" << std::endl;
                uv_read_start(connection->handle, AllocBuffer, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
                    Session::GetInstance()->OnMessageRead(stream, nread, buff);
                });
                handle_ = connection->handle;

                const int STDIN_DESCRIPTOR = 0;
                uv_pipe_init(&loop_, &user_in_, false);
                uv_pipe_open(&user_in_, STDIN_DESCRIPTOR);
                uv_read_start((uv_stream_t*)&user_in_, AllocBuffer, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
                    Session::GetInstance()->ReadInput(stream, nread, buff);
                });
            } else{
                std::cerr << "Connection Unsuccessful" << std::endl;
                std::cerr << uv_strerror(status) << std::endl;
            }
        }
    }
}