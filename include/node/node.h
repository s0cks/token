#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include <uv.h>
#include <memory>
#include <map>
#include "bytes.h"
#include "session.h"

namespace Token{
    namespace Node{
        class Server{
        private:
            typedef std::map<uv_stream_t*, Session> SessionMap;

            static const size_t MAX_BUFF_SIZE = 4096;

            uv_loop_t loop_;
            uv_tcp_t server_;
            ByteBuffer read_buff_;
            bool running_;
            SessionMap sessions_;

            void OnNewConnection(uv_stream_t* server, int status);
            void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
            void OnMessageSent(uv_write_t* stream, int status);
            void Send(uv_stream_t* conn, Message* msg);

            void RemoveClient(uv_stream_t* client){
                //TODO: Implement
            }

            Server():
                sessions_(),
                running_(false),
                server_(),
                loop_(),
                read_buff_(MAX_BUFF_SIZE){}
            ~Server(){}
        public:
            static Server* GetInstance();
            static int Initialize(int port);

            ByteBuffer* GetReadBuffer(){
                return &read_buff_;
            }

            bool IsRunning() const{
                return running_;
            }
        };
    }
}

#endif //TOKEN_NODE_H