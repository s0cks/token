#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <uv.h>
#include "peer.h"
#include "session.h"

namespace Token{
    class BlockChainServer{
    private:
        pthread_t thread_;
        uv_loop_t* loop_;
        uv_tcp_t server_;
        std::map<uv_stream_t*, PeerSession*> sessions_;
        std::vector<PeerClient*> peers_;

        //Async Stuffs?
        uv_async_t broadcast_;

        static void Register(PeerClient* session); //TODO: Refactor
        static void Disconnect(PeerSession* session); //TODO: Refactor
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff);
        static void OnBroadcast(uv_async_t* handle);
        static void* ServerThread(void* data);

        BlockChainServer();

        friend class PeerClient;
    public:
        ~BlockChainServer();

        int Start(int port);

        static BlockChainServer* GetInstance();
        static bool Broadcast(Message* msg);
        static bool AsyncBroadcast(Message* msg);
        static bool ConnectToPeer(const std::string& address, uint32_t port);
        static bool GetPeerList(Messages::PeerList& peers);
        static int Initialize(int port);
        static int ShutdownAndWait();
    };
}

#endif //TOKEN_SERVER_H