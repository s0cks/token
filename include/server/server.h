#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#ifdef TOKEN_ENABLE_SERVER

#include <uv.h>
#include <sstream>
#include "vthread.h"
#include "configuration.h"

namespace Token{
#define FOR_EACH_SERVER_STATE(V) \
    V(Starting) \
    V(Running) \
    V(Synchronizing) \
    V(Stopping) \
    V(Stopped)

#define FOR_EACH_SERVER_STATUS(V) \
    V(Ok)                         \
    V(Warning)                    \
    V(Error)

  class Server{
   public:
    enum State{
#define DEFINE_SERVER_STATE(Name) k##Name,
      FOR_EACH_SERVER_STATE(DEFINE_SERVER_STATE)
#undef DEFINE_SERVER_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
                case Server::k##Name: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_SERVER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }

    enum Status{
#define DEFINE_SERVER_STATUS(Name) k##Name,
      FOR_EACH_SERVER_STATUS(DEFINE_SERVER_STATUS)
#undef DEFINE_SERVER_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
                case Server::k##Name: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_SERVER_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }
   private:
    Server() = delete;

    static uv_tcp_t* GetHandle();
    static void SetStatus(Status status);
    static void SetState(State state);
    static void OnClose(uv_handle_t* handle);
    static void OnWalk(uv_handle_t* handle, void* data);
    static void HandleTerminateCallback(uv_async_t* handle);
    static void OnNewConnection(uv_stream_t* stream, int status);
    static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    static void HandleServerThread(uword parameter);
   public:
    ~Server() = delete;

    static State GetState();
    static Status GetStatus();
    static void WaitForState(State state);
    static bool Start();
    static bool Stop();

    static inline UUID GetID(){
      return BlockChainConfiguration::GetSererID();
    }

#define DEFINE_STATE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == Server::k##Name; }
    FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

#define DEFINE_STATUS_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == Server::k##Name; }
    FOR_EACH_SERVER_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK

#define ENVIRONMENT_TOKEN_CALLBACK_ADDRESS "TOKEN_CALLBACK_ADDRESS"

    static inline NodeAddress
    GetCallbackAddress(){
      return NodeAddress::ResolveAddress(GetEnvironmentVariable(ENVIRONMENT_TOKEN_CALLBACK_ADDRESS));
    }
  };
}

#endif//TOKEN_ENABLE_SERVER
#endif //TOKEN_SERVER_H