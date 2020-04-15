#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <uv.h>
#include <http_parser.h>

namespace Token{
    class BlockChainHttpServer{
    private:
        pthread_t thread_;
        uv_tcp_t handle_;
        http_parser parser_;
    public:
        BlockChainHttpServer():
            thread_(),
            handle_(),
            parser_(){

        }
    };
}

#endif //TOKEN_SERVER_H