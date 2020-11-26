#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uv.h>
#include <mutex>
#include <uuid/uuid.h>
#include <condition_variable>

#include "buffer.h"
#include "message.h"

namespace Token{
#define FOR_EACH_SESSION_STATE(V) \
    V(Connecting)                 \
    V(Connected)                  \
    V(Disconnected)

#define FOR_EACH_SESSION_STATUS(V) \
    V(Ok)                          \
    V(Warning)                     \
    V(Error)

    class Session : public Object{
    public:
        static const uint32_t kHeartbeatIntervalMilliseconds = 30 * 1000; //TODO: remove Session::kHeartbeatIntervalMilliseconds
        static const uint32_t kHeartbeatTimeoutMilliseconds = 1 * 60 * 1000; //TODO: remove Session::kHeartbeatTimeoutMilliseconds
        static const intptr_t kBufferSize = 4096;

        enum State{
#define DEFINE_STATE(Name) k##Name,
            FOR_EACH_SESSION_STATE(DEFINE_STATE)
#undef DEFINE_STATE
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
#define DEFINE_STATE_CHECK(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_SESSION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
                default:
                    stream << "Unknown State";
                    return stream;
            }
        }

        enum Status{
#define DEFINE_STATUS(Name) k##Name,
            FOR_EACH_SESSION_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
        };

        friend std::ostream& operator<<(std::ostream& stream, const Status& status){
            switch(status){
#define DEFINE_STATUS_CHECK(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
            FOR_EACH_SESSION_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK
                default:
                    stream << "Unknown Status";
                    return stream;
            }
        }
    private:
        std::mutex mutex_;
        std::condition_variable cond_;
        State state_;
        Status status_;
        uv_loop_t* loop_;
        uv_tcp_t handle_;
        Buffer* rbuffer_;
        Buffer* wbuffer_;
    protected:
        Session(uv_loop_t* loop):
            Object(),
            mutex_(),
            cond_(),
            state_(State::kDisconnected),
            status_(Status::kOk),
            loop_(loop),
            handle_(),
            rbuffer_(nullptr),
            wbuffer_(nullptr){
            SetType(Type::kSessionType);
            handle_.data = this;

            int err;
            if((err = uv_tcp_init(loop, &handle_)) != 0){
                LOG(WARNING) << "couldn't initialize the session handle: " << uv_strerror(err);
                return;
            }

            WriteBarrier(&rbuffer_, Buffer::NewInstance(kBufferSize));
            WriteBarrier(&wbuffer_, Buffer::NewInstance(kBufferSize));
        }

        bool Accept(WeakObjectPointerVisitor* vis){
            if(!vis->Visit(&rbuffer_)){
                LOG(WARNING) << "couldn't visit the session read buffer";
                return false;
            }
            if(!vis->Visit(&wbuffer_)){
                LOG(WARNING) << "couldn't visit the session write buffer";
                return false;
            }
            return true;
        }

        uv_tcp_t* GetHandle() const{
            return (uv_tcp_t*)&handle_;
        }

        uv_stream_t* GetStream() const{
            return (uv_stream_t*)&handle_;
        }

        uv_loop_t* GetLoop() const{
            return loop_;
        }

        Handle<Buffer> GetReadBuffer() const{
            return rbuffer_;
        }

        Handle<Buffer> GetWriteBuffer() const{
            return wbuffer_;
        }

        void SetState(State state);
        void SetStatus(Status status);
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnMessageSent(uv_write_t* req, int status);
    public:
        State GetState();
        Status GetStatus();
        void Send(const Handle<Message>& msg);
        void Send(std::vector<Handle<Message>>& messages);

#define DEFINE_SEND_MESSAGE(Name) \
        void Send(const Handle<Name##Message>& msg){ Send(msg.CastTo<Message>()); }
        FOR_EACH_MESSAGE_TYPE(DEFINE_SEND_MESSAGE)
#undef DEFINE_SEND_MESSAGE

#define DEFINE_STATE_CHECK(Name) \
        bool Is##Name(){ return GetState() == State::k##Name; }
        FOR_EACH_SESSION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

#define DEFINE_STATUS_CHECK(Name) \
        bool Is##Name(){ return GetStatus() == Status::k##Name; }
        FOR_EACH_SESSION_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK
    };
}

#endif //TOKEN_SESSION_H