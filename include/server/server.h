#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#ifdef TOKEN_ENABLE_SERVER

#include <uv.h>
#include <sstream>
#include "vthread.h"
#include "configuration.h"
#include "server/channel.h"
#include "atomic/relaxed_atomic.h"

#define DEFAULT_SERVER_BACKLOG 100

namespace Token{
#define FOR_EACH_SERVER_STATE(V) \
    V(Starting) \
    V(Running) \
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
      switch (state){
#define DEFINE_TOSTRING(Name) \
        case Server::k##Name##State: \
            return stream << #Name;
        FOR_EACH_SERVER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:return stream << "Unknown";
      }
    }
   protected:
    pthread_t thread_;
    uv_loop_t* loop_;
    uv_tcp_t handle_;
    RelaxedAtomic<State> state_;

    Server(uv_loop_t* loop):
      thread_(),
      loop_(loop),
      handle_(),
      state_(Server::kStoppedState){
      int err;
      if((err = uv_tcp_init(GetLoop(), GetHandle())) != 0){
        LOG(WARNING) << "server initialize error: " << uv_strerror(err);
        return;
      }
    }

    void SetState(const State& state){
      state_ = state;
    }

    static void HandleServerThread(uword parameter);
    static void OnNewConnection(uv_stream_t* stream, int status);
    static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);

    static inline int
    Bind(uv_tcp_t* server, const int32_t port){
      sockaddr_in bind_address;
      uv_ip4_addr("0.0.0.0", port, &bind_address);
      return uv_tcp_bind(server, (struct sockaddr*)&bind_address, 0);
    }

    static inline int
    Listen(uv_stream_t* server, uv_connection_cb on_connect, const int backlog=DEFAULT_SERVER_BACKLOG){
      return uv_listen(server, backlog, on_connect);
    }

    static inline int
    Accept(uv_stream_t* server, Channel& channel){
      return uv_accept(server, channel.GetStream());
    }

    static inline int
    ReadStart(Channel& channel, uv_alloc_cb on_alloc, uv_read_cb on_read){
      return uv_read_start(channel.GetStream(), on_alloc, on_read);
    }
   public:
    virtual ~Server() = default;

    bool JoinThread();
    bool StartThread();

    pthread_t GetThread() const{
      return thread_;
    }

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    uv_tcp_t* GetHandle(){
      return &handle_;
    }

    uv_stream_t* GetStream(){
      return (uv_stream_t*)&handle_;
    }

    State GetState() const{
      return state_;
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name(){ return GetState() == Server::k##Name##State; }
    FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };

/*//    static uv_tcp_t* GetHandle();
//    static void SetState(const State& state);
//    static void OnClose(uv_handle_t* handle);
//    static void OnWalk(uv_handle_t* handle, void* data);
//    static void HandleTerminateCallback(uv_async_t* handle);
//    static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
//    static void HandleServerThread(uword parameter);
//    static bool SendShutdown();
//   public:
//    ~Server() = delete;
//    static State GetState();
//    static bool JoinThread();
//    static bool StartThread();
//
//    static inline bool
//    Shutdown(){
//      return SendShutdown() && JoinThread();
//    }
//
//    static inline UUID GetID(){
//      return BlockChainConfiguration::GetSererID();
//    }
//
//#define DEFINE_CHECK(Name) \
//    static inline bool Is##Name##State(){ return GetState() == Server::State::k##Name##State; }
//    FOR_EACH_SERVER_STATE(DEFINE_CHECK)
//#undef DEFINE_CHECK
//
//#define ENVIRONMENT_TOKEN_CALLBACK_ADDRESS "TOKEN_CALLBACK_ADDRESS"
//
//    static inline NodeAddress
//    GetCallbackAddress(){
//      return NodeAddress::ResolveAddress(GetEnvironmentVariable(ENVIRONMENT_TOKEN_CALLBACK_ADDRESS));
//    }
//  };*/
}

#endif//TOKEN_ENABLE_SERVER
#endif //TOKEN_SERVER_H