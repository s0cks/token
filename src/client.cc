#include "client.h"
#include <iostream>

namespace Token{
    namespace Client{
        static void
        AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
            ByteBuffer* read = Session::GetInstance()->GetReadBuffer();
            *buff = uv_buf_init((char*) read->GetBytes(), read->Size());
        }

        Message* Session::GetMessage(){
            uint32_t type = GetReadBuffer()->GetInt();
            switch(type){
                case Message::Type::kBlockMessage: return BlockMessage::Decode(GetReadBuffer());
                default: return nullptr;
            }
        }

        void Session::OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
            if(nread == UV_EOF){
                std::cerr << "Disconnected" << std::endl;
                uv_read_stop(stream);
            } else if(nread > 0){
                GetReadBuffer()->PutBytes((uint8_t*) buff->base, buff->len);
                Message* msg = GetMessage();
                std::cout << "Read: " << buff->len << std::endl;
                if(msg->IsBlock()){
                    std::cout << "Reading head" << std::endl;
                    BlockMessage* blockMsg = msg->AsBlock();
                    std::cout << "<HEAD>" << std::endl;
                    std::cout << (*blockMsg->GetBlock()) << std::endl;
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

        void Session::OnMessageSend(uv_write_t *request, int status){
            if(status == 0){

            } else{
                std::cerr << "Error writing message" << std::endl;
            }
        }

        void Session::Send(Token::Message* msg){
            std::cout << "Sending " << msg->GetName() << std::endl;
            ByteBuffer bb;
            msg->Encode(&bb);
            bb.Put('\0');
            uv_buf_t buff = uv_buf_init((char*) bb.GetBytes(), bb.WrittenBytes());
            uv_write_t write;
            uv_write(&write, handle_, &buff, 1, [](uv_write_t* req, int status){
                Session::GetInstance()->OnMessageSend(req, status);
            });
        }

        void Session::OnConnect(uv_connect_t *connection, int status){
            if(status == 0){
                std::cout << "Connected" << std::endl;
                handle_ = connection->handle;
                uv_read_start(connection->handle, AllocBuffer, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
                    Session::GetInstance()->OnMessageRead(stream, nread, buff);
                });
                handle_ = connection->handle;
                
                GetHeadMessage msg;
                Send(&msg);
            } else{
                std::cerr << "Connection Unsuccessful" << std::endl;
                std::cerr << uv_strerror(status) << std::endl;
            }
        }
    }
}