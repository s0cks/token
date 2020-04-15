#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <uv.h>
#include <http_parser.h>
#include "common.h"
#include "request.h"

namespace Token{
    class BlockChainHttpServer{
    private:
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnShutdown(uv_shutdown_t* req, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void HandleHealthRequest(uv_work_t* req);
        static void AfterHandleRequest(uv_work_t* req, int status);
        static void* ServerThread(void* data);
    public:
        ~BlockChainHttpServer(){
            //TODO: Shutdown?
        }

        static bool StartServer();
    };
}

#endif //TOKEN_SERVER_H