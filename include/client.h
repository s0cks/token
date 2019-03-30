#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include <uv.h>
#include <memory.h>
#include "bytes.h"
#include "message.h"

namespace Token{
    namespace Client{
        class Session{
        private:
            uv_loop_t loop_;
            uv_tcp_t stream_;
            uv_connect_t conn_;
            uv_stream_t* handle_;
            uv_pipe_t user_in_;
            struct sockaddr_in addr_;
            ByteBuffer read_buff_;
            bool running_;

            Message* GetMessage();
            void OnConnect(uv_connect_t* connection, int status);
            void OnMessageRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
            void OnMessageSend(uv_write_t* request, int status);
            void ReadInput(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);

            Session():
                running_(false),
                handle_(nullptr),
                read_buff_(4096),
                addr_(),
                conn_(),
                user_in_(),
                stream_(){
                uv_loop_init(&loop_);
            }
            ~Session(){
                uv_loop_close(&loop_);
            }
        public:
            static Session* GetInstance();
            static void Initialize(int port, const std::string& addr);

            ByteBuffer*
            GetReadBuffer() {
                return &read_buff_;
            }

            void Send(Message* msg);

            bool IsRunning() const{
                return running_;
            }
        };
    }
}

#endif //TOKEN_CLIENT_H