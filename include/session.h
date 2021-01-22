#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#ifdef TOKEN_ENABLE_SERVER

#include <uv.h>
#include <mutex>
#include <condition_variable>
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

  class Message;
  class Session : public Object{
   public:
    static const uint32_t
      kHeartbeatIntervalMilliseconds = 30 * 1000; //TODO: remove Session::kHeartbeatIntervalMilliseconds
    static const uint32_t
      kHeartbeatTimeoutMilliseconds = 1 * 60 * 1000; //TODO: remove Session::kHeartbeatTimeoutMilliseconds
    static const intptr_t kBufferSize = 65536;

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
        default:stream << "Unknown State";
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
        default:stream << "Unknown Status";
          return stream;
      }
    }
   protected:
    std::mutex mutex_;
    std::condition_variable cond_;
    State state_;
    Status status_;
    uv_loop_t* loop_;
    uv_tcp_t handle_;

    Session(uv_loop_t* loop);

    uv_tcp_t* GetHandle() const{
      return (uv_tcp_t*) &handle_;
    }

    uv_stream_t* GetStream() const{
      return (uv_stream_t*) &handle_;
    }

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    void SetState(State state);
    void SetStatus(Status status);
    bool WaitForState(State state, intptr_t timeout = 0);
    bool WaitForStatus(Status status, intptr_t timeout = 0);
    static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
    static void OnMessageSent(uv_write_t* req, int status);
   public:
    State GetState();
    Status GetStatus();
    void Send(const MessagePtr& msg);
    void Send(MessageList& messages);

#define DEFINE_STATE_CHECK(Name) \
        bool Is##Name(){ return GetState() == State::k##Name; }
    FOR_EACH_SESSION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

#define DEFINE_STATUS_CHECK(Name) \
        bool Is##Name(){ return GetStatus() == Status::k##Name; }
    FOR_EACH_SESSION_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK

#define DEFINE_STATE_WAITER(Name) \
        bool WaitFor##Name##State(int64_t timeout=30){ return WaitForState(State::k##Name, timeout); }
    FOR_EACH_SESSION_STATE(DEFINE_STATE_WAITER)
#undef DEFINE_STATE_WAITER
  };
}

#endif//TOKEN_ENABLE_SERVER
#endif //TOKEN_SESSION_H