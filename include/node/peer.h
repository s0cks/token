#ifndef TOKEN_PEER_H
#define TOKEN_PEER_H

#include "common.h"
#include "message.h"
#include "session.h"

namespace Token{
    class PeerSession : public Session{
    private:
        pthread_t thread_;
        uv_connect_t conn_;
        uv_tcp_t socket_;
        std::string node_id_;
        NodeAddress node_addr_;

        pthread_mutex_t gd_mutex_;
        pthread_cond_t gd_cond_;
        uint256_t gd_hash_;

        static void* PeerSessionThread(void* data);
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnMessageSent(uv_write_t *req, int status);

#define DECLARE_HANDLER(Name) \
    static void Handle##Name##Message(uv_work_t* handle);
    FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLER);
        static void AfterHandleMessage(uv_work_t* handle, int status);

        static void HandleGetDataTask(uv_work_t* handle);
        static void AfterHandleGetDataTask(uv_work_t* handle, int status);
    protected:
        uv_stream_t* GetStream() const{
            return conn_.handle;
        }
    public:
        PeerSession(const NodeAddress& addr):
            thread_(),
            conn_(),
            node_addr_(addr),
            node_id_(){
            socket_.data = this;
            conn_.data = this;
        }
       ~PeerSession(){}

        NodeInfo GetInfo() const{
            return NodeInfo(node_id_, node_addr_);
        }

        NodeAddress GetAddress() const{
            return node_addr_;
        }

        std::string GetID() const{
            return node_id_;
        }

        void Send(const Message& msg);

        bool Connect(){
            return pthread_create(&thread_, NULL, &PeerSessionThread, (void*)this) == 0;
        }
    };
}

#endif //TOKEN_PEER_H