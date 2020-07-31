#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uuid.h>
#include <uv.h>
#include <mutex>
#include <condition_variable>
#include "message.h"

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
        uuid_t uuid_;
        NodeAddress address_;
        State state_;

        Session(const NodeAddress& address):
            mutex_(),
            cond_(),
            state_(kDisconnected),
            uuid_(),
            address_(address){
            uuid_generate_time_safe(uuid_);
        }

        Session(uv_tcp_t* handle):
            mutex_(),
            cond_(),
            state_(kDisconnected),
            uuid_(),
            address_(handle){
            uuid_generate_time_safe(uuid_);
        }

        virtual uv_stream_t* GetStream() = 0;
        void SetState(State state);
        void SetID(const std::string& id);
        void OnItemReceived(const InventoryItem& item);

        friend class Server;
    public:
        std::string GetID() const{
            char uuid_str[37];
            uuid_unparse(uuid_, uuid_str);
            return std::string(uuid_str);
        }

        NodeAddress GetAddress() const{
            return address_;
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
        void WaitForItem(const InventoryItem& item);
        void Send(std::vector<Handle<Message>>& messages);
        void Send(const Handle<Message>& msg);

#define DECLARE_MESSAGE_SEND(Name) \
        inline void Send(const Handle<Name##Message>& msg){ \
            Send(msg.CastTo<Message>()); \
        }
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_SEND)
#undef DECLARE_MESSAGE_SEND

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(const Handle<HandleMessageTask>& task) = 0;
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
            Session(&handle_){
            handle_.data = this;
            heartbeat_.data = this;
        }

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&handle_;
        }

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(const Handle<HandleMessageTask>& task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

        friend class Server;
    public:
        ~NodeSession(){}
    };
}

#endif //TOKEN_SESSION_H