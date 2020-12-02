#ifndef TOKEN_PEER_SESSION_H
#define TOKEN_PEER_SESSION_H

#include "peer.h"
#include "block.h"
#include "session.h"

namespace Token{
    class HandleMessageTask;
    class PeerSession : public ThreadedSession{
        friend class Server;
    private:
        Peer info_;
        BlockHeader head_;

        uv_timer_t hb_timer_; //TODO: remove PeerSession::hb_timer_
        uv_timer_t hb_timeout_; //TODO: remove PeerSession::hb_timeout_
        uv_async_t shutdown_; //TODO: remove PeerSession::shutdown_

        PeerSession(uv_loop_t* loop, const NodeAddress& address):
            ThreadedSession(loop),
            info_(UUID(), address),
            head_(),
            hb_timer_(),
            hb_timeout_(),
            shutdown_(){}

        void SetInfo(const Peer& info){
            info_ = info;
        }

        static void OnConnect(uv_connect_t* conn, int status);
        static void OnShutdown(uv_async_t* handle);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void OnHeartbeatTick(uv_timer_t* handle);
        static void OnHeartbeatTimeout(uv_timer_t* handle);
#define DECLARE_MESSAGE_HANDLER(Name) \
        static void Handle##Name##Message(HandleMessageTask* task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
        static void* SessionThread(void* data);
    public:
        Peer GetInfo() const{
            return info_;
        }

        UUID GetID() const{
            return info_.GetID();
        }

        NodeAddress GetAddress() const{
            return info_.GetAddress();
        }

        bool Connect(){
            int err;
            if((err = pthread_create(&thread_, NULL, &SessionThread, this)) != 0){
                LOG(WARNING) << "couldn't start session thread: " << strerror(err);
                return false;
            }
            return true;
        }

        static PeerSession* NewInstance(uv_loop_t* loop, const NodeAddress& address){
            return new PeerSession(loop, address);
        }
    };
}

#endif //TOKEN_PEER_SESSION_H