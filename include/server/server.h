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

  class Server{
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_SERVER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case Server::k##Name##State: \
            return stream << #Name;
        FOR_EACH_SERVER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:return stream << "Unknown";
      }
    }
   private:
    Server() = delete;

    static uv_tcp_t* GetHandle();
    static void SetState(const State& state);
    static void OnClose(uv_handle_t* handle);
    static void OnWalk(uv_handle_t* handle, void* data);
    static void HandleTerminateCallback(uv_async_t* handle);
    static void OnNewConnection(uv_stream_t* stream, int status);
    static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    static void HandleServerThread(uword parameter);
    static bool SendShutdown();
   public:
    ~Server() = delete;
    static State GetState();
    static bool JoinThread();
    static bool StartThread();

    static inline bool
    Shutdown(){
      return SendShutdown() && JoinThread();
    }

    static inline UUID GetID(){
      return BlockChainConfiguration::GetSererID();
    }

#define DEFINE_CHECK(Name) \
    static inline bool Is##Name##State(){ return GetState() == Server::State::k##Name##State; }
    FOR_EACH_SERVER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

#define ENVIRONMENT_TOKEN_CALLBACK_ADDRESS "TOKEN_CALLBACK_ADDRESS"

    static inline NodeAddress
    GetCallbackAddress(){
      return NodeAddress::ResolveAddress(GetEnvironmentVariable(ENVIRONMENT_TOKEN_CALLBACK_ADDRESS));
    }
  };
}

#endif//TOKEN_ENABLE_SERVER
#endif //TOKEN_SERVER_H