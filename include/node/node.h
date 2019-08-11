#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include <uv.h>
#include "common.h"
#include "bytes.h"
#include "session.h"

namespace Token{
    class BlockChainServer{
    private:
        static const size_t MAX_BUFF_SIZE = 4096;

        pthread_t thread_;
        uv_loop_t* loop_;
        uv_tcp_t server_;
        uv_async_t send_handle_;
        uv_async_t shutdown_handle_;
        int port_;

        ByteBuffer read_buff_;
        bool running_;
        std::map<uv_stream_t*, Session*> sessions_;

        inline void
        Register(PeerSession* peer){
            sessions_.insert({ peer->GetStream(), peer });
        }

        inline void
        GetPeers(std::vector<PeerSession*>& peers){
            for(auto& it : sessions_){
                if(it.second->IsPeerSession()){
                    peers.push_back(it.second->AsPeerSession());
                }
            }
        }

        inline void
        SetPort(int port){
            port_ = port;
        }

        BlockChainServer();
        friend class BlockChain;
        friend class ClientSession;
        friend class PeerSession;
    protected:
        void OnNewConnection(uv_stream_t* server, int status);
        void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        void OnMessageSent(uv_write_t* stream, int status);
        void Send(uv_stream_t* conn, Message* msg);
        void BroadcastToPeers(uv_stream_t* from, Message* msg);
    public:
        ~BlockChainServer(){}

        void Broadcast(uv_stream_t* from, Message* msg);
        void AddPeer(const std::string& addr, int port);

        ByteBuffer* GetReadBuffer(){
            return &read_buff_;
        }

        bool IsRunning() const{
            return running_;
        }

        int GetPort() const{
            return port_;
        }

        int Start();
        void Shutdown();
    };
}

#endif //TOKEN_NODE_H