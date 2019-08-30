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
        std::map<std::string, PeerClient*> peers_;

        //Async Stuffs?
        uv_async_t broadcast_;

        static void Register(PeerClient* session);
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

        static bool HasPeer(std::string key);
        static bool GetPeers(std::vector<std::string>& peers);
        static bool GetPeerList(std::vector<PeerClient*>& peers);
        static bool Broadcast(Message* msg);
        static bool AsyncBroadcast(Message* msg);
        static BlockChainServer* GetInstance();
        static int GetPeerCount();
        static int AddPeer(const std::string& address, int port);
        static int Initialize(int port);
        static int ShutdownAndWait();
    };
}

#endif //TOKEN_SERVER_H