#ifndef TOKEN_REST_SERVICE_H
#define TOKEN_REST_SERVICE_H

#ifdef TOKEN_ENABLE_REST_SERVICE

#include <uv.h>
#include "vthread.h"
#include "http/router.h"
#include "http/session.h"
#include "http/request.h"
#include "http/service.h"
#include "http/controller.h"

namespace Token{
#define FOR_EACH_REST_SERVICE_STATE(V) \
    V(Starting)                        \
    V(Running)                         \
    V(Stopping)                        \
    V(Stopped)

#define FOR_EACH_REST_SERVICE_STATUS(V) \
    V(Ok)                               \
    V(Warning)                          \
    V(Error)

  class RestService : Thread, HttpService{
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_REST_SERVICE_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_REST_SERVICE_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }

    enum Status{
#define DEFINE_STATUS(Name) k##Name,
      FOR_EACH_REST_SERVICE_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name;  \
                    return stream;
        FOR_EACH_REST_SERVICE_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }
   private:
    RestService() = delete;

    static void SetState(State state);
    static void SetStatus(Status status);
    static void OnNewConnection(uv_stream_t* stream, int status);
    static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
    static void OnShutdown(uv_async_t* handle);
    static void HandleServiceThread(uword parameter);
   public:
    ~RestService() = delete;

    static State GetState();
    static Status GetStatus();
    static bool Start();
    static bool Stop();
    static void WaitForState(State state);

#define DEFINE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == State::k##Name; }
    FOR_EACH_REST_SERVICE_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

#define DEFINE_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == Status::k##Name; }
    FOR_EACH_REST_SERVICE_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif//TOKEN_ENABLE_REST_SERVICE

#endif //TOKEN_REST_SERVICE_H