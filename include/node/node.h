#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include <pthread.h>
#include <string>
#include <vector>
#include <cstdint>
#include <uv.h>
#include "session.h"

namespace Token{
    //TODO: establish timer for ping
    class BlockChainServer{
    private:
        pthread_t thread_;
        std::map<uv_stream_t*, Session*> sessions_; //TODO fix
        std::list<Session*> peers_;

        BlockChainServer();

        static BlockChainServer* GetInstance();
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void* ServerThread(void* data);

        static void HandleAsyncBroadcastHead(uv_async_t* handle);
        static void HandleBroadcastBlock(uv_work_t* req);

        static void HandleVerack(uv_work_t* req);
        static void HandleBlock(uv_work_t* req);
        static void HandleTransaction(uv_work_t* req);
        static void HandleVersion(uv_work_t* req);
        static void HandleGetData(uv_work_t* req);
        static void HandleGetBlocks(uv_work_t* req);

        static void AfterBroadcastBlock(uv_work_t* req, int status);
        static void AfterHandleMessage(uv_work_t* req, int status);
    public:
        ~BlockChainServer(){}

        static bool StartServer();
        static void ConnectToPeer(const std::string& address, uint16_t port);
        static void BroadcastBlockToPeers(Block* block);

        static void GetConnectedPeers(std::list<Session*>& peers){
            for(auto& it : GetInstance()->peers_){
                if(it->IsConnected()) peers.push_back(it);
            }
        }

        static void WaitForShutdown();

        // static bool ShutdownServer();
        // static bool ShutdownServerAndWait();
        // static void Broadcast(Message* msg);
    };
}

#endif //TOKEN_NODE_H