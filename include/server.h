#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <uv.h>
#include "session.h"

namespace Token{
    class BlockChainServer{
    private:
        pthread_t thread_;

        uv_loop_t* loop_;
        uv_tcp_t server_;
        std::map<uv_stream_t*, Session*> sessions_;

        BlockChainServer();
    public:
        ~BlockChainServer();

        static int Initialize(int port);
        static int ShutdownAndWait();
    };
}

#endif //TOKEN_SERVER_H