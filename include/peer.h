#ifndef TOKEN_PEER_H
#define TOKEN_PEER_H

#include <uuid/uuid.h>
#include "address.h"
#include "session.h"

namespace Token{
    class HandleMessageTask;

    class PeerSession : public Session{
        friend class Server;
    private:
        pthread_t thread_;
        UUID id_;
        NodeAddress address_;
        BlockHeader head_;

        uv_timer_t hb_timer_; //TODO: remove PeerSession::hb_timer_
        uv_timer_t hb_timeout_; //TODO: remove PeerSession::hb_timeout_
        uv_async_t shutdown_; //TODO: remove PeerSession::shutdown_

        PeerSession(uv_loop_t* loop, const NodeAddress& address):
            Session(loop),
            thread_(),
            id_(),
            address_(address),
            head_(),
            hb_timer_(),
            hb_timeout_(),
            shutdown_(){
            SetType(Type::kPeerSessionType);
        }

        void SetHead(const BlockHeader& head){
            head_ = head;
        }

        void SetID(const UUID& id){
            id_ = id;
        }

        static void* PeerSessionThread(void* data);
        static void OnConnect(uv_connect_t* conn, int status);
        static void OnShutdown(uv_async_t* handle);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void OnHeartbeatTick(uv_timer_t* handle);
        static void OnHeartbeatTimeout(uv_timer_t* handle);
#define DECLARE_MESSAGE_HANDLER(Name) \
        static void Handle##Name##Message(const Handle<HandleMessageTask>& task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
    public:
        UUID GetID() const{
            return id_;
        }

        NodeAddress GetAddress() const{
            return address_;
        }

        bool Connect();
        bool Disconnect();

        static Handle<PeerSession> NewInstance(uv_loop_t* loop, const NodeAddress& address){
            return new PeerSession(loop, address);
        }
    };
}

#endif //TOKEN_PEER_H