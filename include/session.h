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
    class SessionInfo;
    class HandleMessageTask;
    class Session{
        friend class BlockChainClient; //TODO: revoke access to Session from BlockChainClient
    public:
        static const uint32_t kHeartbeatIntervalMilliseconds = 30 * 1000;
        static const uint32_t kHeartbeatTimeoutMilliseconds = 1 * 60 * 1000;
        static const intptr_t kBufferSize = 65536; //bad?

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
        State state_;
        UUID uuid_;
        NodeAddress address_;
        BlockHeader head_;
        Message* next_;

        Session(const NodeAddress& address):
            mutex_(),
            cond_(),
            state_(kDisconnected),
            uuid_(),
            address_(address),
            head_(),
            next_(nullptr){}
        Session(uv_tcp_t* handle):
            mutex_(),
            cond_(),
            state_(kDisconnected),
            uuid_(),
            address_(handle),
            head_(),
            next_(nullptr){}

        virtual uv_stream_t* GetStream() = 0;
        void SetState(State state);
        void OnItemReceived(const InventoryItem& item);
        void OnNextMessageReceived(const Handle<Message>& msg);
        Handle<Message> GetNextMessage();

        void SetID(const UUID& uuid){
            uuid_ = uuid;
        }

        void SetHead(const BlockHeader& head){
            head_ = head;
        }

        friend class Server;
    public:
        BlockHeader GetHead() const{
            return head_;
        }

        UUID GetID() const{
            return uuid_;
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
        void WaitForNextMessage();
        void WaitForNextMessage(Message::MessageType type);
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
}

#endif //TOKEN_SESSION_H