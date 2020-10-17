#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uuid/uuid.h>
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
        friend class BlockChainClient; //TODO: revoke access to Session from BlockChainClient
    public:
        static const uint32_t kHeartbeatIntervalMilliseconds = 30 * 1000;
        static const uint32_t kHeartbeatTimeoutMilliseconds = 1 * 60 * 1000;
        static const intptr_t kBufferSize = 4096*2;//bad?

        enum State{
            kDisconnected = 0,
            kConnecting,
            kConnected
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
                case State::kConnecting:
                    stream << "Connecting";
                    return stream;
                case State::kConnected:
                    stream << "Connected";
                    return stream;
                case State::kDisconnected:
                    stream << "Disconnected";
                    return stream;
                default:
                    stream << "Unknown";
                    return stream;
            }
        }
    protected:
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnMessageSent(uv_write_t* req, int status);

        std::recursive_mutex mutex_;
        std::condition_variable_any cond_;
        uuid_t uuid_;
        NodeAddress address_;
        State state_;

        Message* next_;

        Session(const NodeAddress& address):
            mutex_(),
            cond_(),
            uuid_(),
            address_(address),
            state_(kDisconnected),
            next_(nullptr){
            uuid_generate_time_safe(uuid_);
        }

        Session(uv_tcp_t* handle):
            mutex_(),
            cond_(),
            uuid_(),
            address_(handle),
            state_(kDisconnected){
            uuid_generate_time_safe(uuid_);
        }

        virtual uv_stream_t* GetStream() = 0;
        void SetState(State state);
        void SetID(const std::string& id);
        void OnItemReceived(const InventoryItem& item);
        void OnNextMessageReceived(const Handle<Message>& msg);
        Handle<Message> GetNextMessage();

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
        void SendInventory(std::vector<InventoryItem>& items);

#define DEFINE_SEND_MESSAGE(Name) \
        void Send(const Handle<Name##Message>& msg){ Send(msg.CastTo<Message>()); }
        FOR_EACH_MESSAGE_TYPE(DEFINE_SEND_MESSAGE)
#undef DEFINE_SEND_MESSAGE

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(const Handle<HandleMessageTask>& task) = 0;
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
    };

    class SessionInfo{
    protected:
        Session* session_;

        SessionInfo(Session* session):
            session_(session){}
    public:
        ~SessionInfo() = default;

        Session* GetSession() const{
            return session_;
        }

        std::string GetID() const{
            return session_->GetID();
        }

        NodeAddress GetAddress() const{
            return session_->GetAddress();
        }

        Session::State GetState() const{
            return session_->GetState();
        }

        bool IsDisconnected() const{
            return GetState() == Session::State::kDisconnected;
        }

        bool IsConnected() const{
            return GetState() == Session::State::kConnected;
        }

        bool IsConnecting() const{
            return GetState() == Session::State::kConnecting;
        }

        void operator=(const SessionInfo& info){
            session_ = info.session_;
        }
    };

    //TODO: add heartbeat?
    class NodeSession : public Session{
    private:
        uv_tcp_t handle_;
        uv_timer_t heartbeat_;

        NodeSession():
            Session(&handle_),
            handle_(),
            heartbeat_(){
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