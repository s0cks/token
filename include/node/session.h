#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <mutex>
#include <condition_variable>
#include "message.h"
#include "node_info.h"

namespace Token{
    //TODO:
    // - create + use Bytes class
    // - scope?
    class HandleMessageTask;
    class Session{
    public:
        static const size_t kBufferSize = 4096;

        enum State{
            kDisconnected = 0,
            kConnecting,
            kConnected
        };
    protected:
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnMessageSent(uv_write_t* req, int status);

        std::recursive_mutex mutex_;
        std::condition_variable_any cond_;
        State state_;
        NodeInfo info_;

        Session(const NodeInfo& info):
            mutex_(),
            cond_(),
            info_(info),
            state_(kDisconnected){}

        virtual uv_stream_t* GetStream() = 0;
        void SetState(State state);

        friend class Node;
    public:
        NodeInfo GetInfo() const{
            return info_;
        }

        std::string GetID() const{
            return info_.GetNodeID();
        }

        NodeAddress GetAddress() const{
            return info_.GetNodeAddress();
        }

        bool IsDisconnected(){
            return GetState() == kDisconnected;
        }

        bool IsConnecting(){
            return GetState() == kConnecting;
        }

        bool IsConnected(){
            return GetState() == kConnected;
        }

        State GetState();
        void WaitForState(State state);
        void Send(Message* msg);
        void Send(std::vector<Message*>& messages);

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(HandleMessageTask* task) = 0;
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
    };

    //TODO: add heartbeat?
    class NodeSession : public Session{
    private:
        uv_tcp_t handle_;
        uv_timer_t heartbeat_;

        NodeSession():
            handle_(),
            heartbeat_(),
            Session(NodeInfo(&handle_)){
            handle_.data = this;
            heartbeat_.data = this;
        }

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&handle_;
        }

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(HandleMessageTask* task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

        friend class Node;
    public:
        ~NodeSession(){}
    };

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
        static void HandleSynchronizeBlocksTask(uv_work_t* handle); //TODO: remove
        static void AfterSynchronizeBlocksTask(uv_work_t* handle, int status); //TODO: remove
    protected:
        virtual uv_stream_t* GetStream(){
            return conn_.handle;
        }

        uv_loop_t* GetLoop() const{
            return socket_.loop;
        }
    public:
        PeerSession(const NodeAddress& addr):
                Session(NodeInfo(addr)),
                thread_(),
                socket_(),
                conn_(){
            socket_.data = this;
            conn_.data = this;
        }
        ~PeerSession(){}

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(HandleMessageTask* task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

        void Shutdown();
        bool Connect();
    };

    class NodeClient : public Session{
    private:
        uv_loop_t* loop_;
        uv_signal_t sigterm_;
        uv_signal_t sigint_;
        uv_tcp_t stream_;
        uv_connect_t connection_;
        uv_pipe_t stdin_;
        uv_pipe_t stdout_;

        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnCommandReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnSignal(uv_signal_t* handle, int signum);

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&stream_;
        }

        inline uv_stream_t*
        GetStdinPipe(){
            return (uv_stream_t*)&stdin_;
        }
    public:
        NodeClient():
                stream_(),
                loop_(uv_loop_new()),
                sigterm_(),
                sigint_(),
                stdin_(),
                stdout_(),
                Session(&stream_){
            stdin_.data = this;
            stdout_.data = this;
            stream_.data = this;
            connection_.data = this;
        }
        ~NodeClient(){}

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(HandleMessageTask* task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

        void Connect(const NodeAddress& addr);
    };
}

#endif //TOKEN_SESSION_H