#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include <uv.h>
#include <memory.h>
#include "bytes.h"
#include "message.h"

namespace Token{
    class Client{
    private:
        uv_loop_t loop_;
        uv_tcp_t stream_;
        uv_connect_t connection_;
        uv_stream_t* handle_;
        uv_pipe_t userin_;
        struct sockaddr_in address_;
        ByteBuffer read_buffer_;
        bool running_;

        Message* GetNextMessage();
        void Initialize(const std::string& address, int port);
        void OnConnect(uv_connect_t* connection, int status);
        void OnMessageRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        void OnMessageSend(uv_write_t* request, int status);
        void Send(Message* msg);

        Client():
            running_(false),
            handle_(NULL),
            read_buffer_(4096),
            address_(),
            connection_(),
            loop_(),
            stream_(){
            uv_loop_init(&loop_);
        }
    public:
        ~Client(){
            uv_loop_close(&loop_);
        }

        static Client* Connect(const std::string& address, int port);

        ByteBuffer*
        GetReadBuffer(){
            return &read_buffer_;
        }

        bool IsRunning() const{
            return running_;
        }
    };
}

#endif //TOKEN_CLIENT_H