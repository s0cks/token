#ifndef TOKEN_PEER_H
#define TOKEN_PEER_H

#include <uv.h>
#include <memory.h>
#include "bytes.h"
#include "message.h"

namespace Token{
    namespace Node{
        class Peer{
        public:
            enum class State{
                kConnected,
                kDisconnected,
            };
        private:
            uv_loop_t* loop_;
            uv_tcp_t stream_;
            uv_connect_t conn_;
            uv_stream_t* handle_;
            struct sockaddr_in addr_;
            std::string address_;
            int port_;
            ByteBuffer read_buffer_;
            State state_;

            void OnConnect(uv_connect_t* conn, int status);
            void OnMessageRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
            void OnMessageSend(uv_write_t* req, int status);

            inline Message*
            GetNextMessage(){
                uint32_t type = GetReadBuffer()->GetInt();
                Message* msg = Message::Decode(type, GetReadBuffer());
                GetReadBuffer()->Clear();
                return msg;
            }

            inline State
            GetState() const{
                return state_;
            }
        public:
            Peer(uv_loop_t* loop, std::string addr, int port):
                loop_(loop),
                state_(State::kDisconnected),
                address_(addr),
                port_(port),
                stream_(),
                conn_(),
                handle_(nullptr),
                addr_(),
                read_buffer_(4096){}
            ~Peer(){}

            void Connect();

            std::string GetAddress() const{
                return address_;
            }

            int GetPort() const{
                return port_;
            }

            ByteBuffer*
            GetReadBuffer(){
                return &read_buffer_;
            }
            
            uv_stream_t*
            GetStream() const{
                return handle_; 
            }

            bool IsConnected() const{
                return GetState() == State::kConnected;
            }

            void Send(Message* msg);
            void SendTo(uv_stream_t* stream, Message* msg);

            friend std::ostream& operator<<(std::ostream& stream, const Peer& p){
                stream << p.GetAddress() << ":" << p.GetPort();
                return stream;
            }
        };
    }
}

#endif //TOKEN_PEER_H