#ifndef TOKEN_PEER_H
#define TOKEN_PEER_H

#include <uv.h>
#include <queue>
#include "message.h"

namespace Token{
    class PeerClient{
        /**
         * Client Side
         *
         * Handles distributing to others
         */
    public:
        enum class State{
            kDisconnected = 0,
            kConnecting,
            kSynchronizing,
            kConnected,
        };
    private:
        uv_loop_t* loop_;
        uv_tcp_t stream_;
        uv_connect_t connection_;
        uv_stream_t* handle_;
        pthread_t thread_;
        std::string address_;
        int port_;
        State state_;
        uv_async_t async_send_;
        std::string last_nonce_;
        std::queue<std::string> block_queue_;

        inline void
        SetState(State state){
            state_ = state;
        }

        inline std::string
        GetLastNonce(){
            return last_nonce_;
        }

        void DownloadBlock(const std::string& hash);
        void SendVersionMessage(const std::string& nonce);
        void SendPeerList();

        int ConnectToPeer();
        bool Handle(uv_stream_t* stream, Message *msg);
        void OnConnect(uv_connect_t* conn, int status);
        void OnMessageRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        void OnMessageSent(uv_write_t* req, int status);

        static void* PeerSessionThread(void* data);
    public:
        PeerClient(const std::string& addr, int port);
        ~PeerClient();

        int GetPort() const{
            return port_;
        }

        std::string GetAddress() const{
            return address_;
        }

        void AsyncSend(Message* msg);
        bool Send(Message *msg);
        bool Connect();
        bool Disconnect();
        std::string ToString() const;

        State GetState() const{
            return state_;
        }

        bool IsDisconnected() const{
            return GetState() == State::kDisconnected;
        }

        bool IsConnected() const{
            return GetState() == State::kConnected;
        }

        bool IsConnecting() const{
            return GetState() == State::kConnecting;
        }

        bool IsSynchronizing() const{
            return GetState() == State::kSynchronizing;
        }
    };
}

#endif //TOKEN_PEER_H