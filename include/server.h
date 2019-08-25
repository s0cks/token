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
        std::map<uv_stream_t*, PeerSession*> sessions_;

        //Async Stuffs?
        uv_async_t broadcast_;

        static void Register(PeerSession* session);
        static void Disconnect(PeerSession* session);
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff);
        static void OnBroadcast(uv_async_t* handle);
        static ByteBuffer* GetReadBuffer();
        static void* ServerThread(void* data);

        BlockChainServer();

        friend class PeerClient;
    public:
        ~BlockChainServer();

        int Start(int port);

        static bool Broadcast(uv_stream_t* stream, Message* msg);
        static BlockChainServer* GetInstance();
        static int AddPeer(const std::string& address, int port);
        static int Initialize(int port);
        static int ShutdownAndWait();
    };
}

#endif //TOKEN_SERVER_H