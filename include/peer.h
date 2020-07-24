#ifndef TOKEN_PEER_H
#define TOKEN_PEER_H

#include <uuid.h>
#include "address.h"
#include "session.h"

namespace Token{
    //TODO: add heartbeat?
    class HandleMessageTask;
    class PeerSession : public Session{
    private:
        pthread_t thread_;
        uv_connect_t conn_;
        uv_tcp_t socket_;
        uv_async_t shutdown_;

        static void* PeerSessionThread(void* data);
        static void OnShutdown(uv_async_t* handle);
        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    protected:
        virtual uv_stream_t* GetStream(){
            return conn_.handle;
        }
    public:
        PeerSession(const NodeAddress& address):
            Session(address),
            thread_(),
            socket_(),
            conn_(){
            socket_.data = this;
            conn_.data = this;
        }
        ~PeerSession(){}

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(const Handle<HandleMessageTask>& task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

        void Disconnect();
        bool Connect();
    };
}

#endif //TOKEN_PEER_H