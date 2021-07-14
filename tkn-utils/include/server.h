#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <uv.h>
#include <sstream>
#include "session.h"

#include "os_thread.h"
#include "atomic/relaxed_atomic.h"

#define DEFAULT_SERVER_BACKLOG 100

namespace token{
#define FOR_EACH_SERVER_STATE(V) \
    V(Starting) \
    V(Running) \
    V(Stopping) \
    V(Stopped)

  typedef int32_t ServerPort;

#define LOG_SERVER(LevelName) \
  LOG(LevelName) << "[server] "

#define DLOG_SERVER(LevelName) \
  DLOG(LevelName) << "[server] "

#define DLOG_SERVER_IF(LevelName, Condition) \
  DLOG_IF(LevelName, Condition) << "[server] "

  template<class SessionType>
  class ServerBase{
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_SERVER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch (state){
#define DEFINE_TOSTRING(Name) \
        case ServerBase::k##Name##State: \
            return stream << #Name;
        FOR_EACH_SERVER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   protected:
    uv_loop_t* loop_;
    uv_tcp_t handle_;
    uv_async_t shutdown_;
    atomic::RelaxedAtomic<State> state_;

    explicit ServerBase(uv_loop_t* loop):
      loop_(loop),
      handle_(),
      shutdown_(),
      state_(ServerBase::kStoppedState){
      shutdown_.data = this;
      handle_.data = this;
      CHECK_UVRESULT(uv_tcp_init(GetLoop(), &handle_), LOG_SERVER(ERROR), "cannot initialize the server handle");
    }

    void SetState(const State& state){
      state_ = state;
    }

    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
      auto session = (SessionBase*)handle->data;
      LOG(INFO) << "allocating buffer of size " << suggested_size << "b for session: " << session->GetUUID();

      buf->base = (char*)malloc(suggested_size);
      buf->len = suggested_size;
    }

    static void
    OnNewConnection(uv_stream_t* stream, int status){
      CHECK_UVRESULT2(ERROR, status, "connect failure");
      DLOG(INFO) << "(" << status << ") accepting new connection....";

      auto server = (ServerBase*)stream->data;
      auto session = server->CreateSession();
      DLOG(INFO) << "creating new session (" << session->GetUUID() << ")....";
      CHECK_UVRESULT2(ERROR, Accept(stream, session), "accept failure");
      DLOG(INFO) << "reading from new session (" << session->GetUUID() << ")....";
      CHECK_UVRESULT2(ERROR, ReadStart(session, &AllocBuffer, &OnMessageReceived), "read failure");
      DLOG(INFO) << "closing connection.";
    }

    static void
    OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
      LOG(INFO) << "received " << nread << " bytes.";
      auto session = ((SessionType*)stream->data); //TODO: Clean this cast up?
      if(nread == UV_EOF){
        LOG(WARNING) << "client disconnected!";
        return;
      } else if(nread < 0){
        LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
        return;
      } else if(nread == 0){
        LOG(WARNING) << "zero message size received";
        return;
      } else if(nread > 65536){
        LOG(ERROR) << "too large of a buffer: " << nread;
        return;
      }

      BufferPtr buffer = internal::CopyBufferFrom((uint8_t *) buff->base, nread);
      session->OnMessageRead(buffer);
    }

    static void OnClose(uv_handle_t* handle){
      DLOG(INFO) << "on-close.";
    }

    static void OnWalk(uv_handle_t* handle, void* arg){
      uv_close(handle, &OnClose);
    }

    static void OnShutdown(uv_async_t* handle){
      uv_walk(handle->loop, &OnWalk, nullptr);
    }

    static inline int
    Bind(uv_tcp_t* server, const int32_t port){
      sockaddr_in bind_address{};
      uv_ip4_addr("0.0.0.0", port, &bind_address);
      return uv_tcp_bind(server, (struct sockaddr*)&bind_address, 0);
    }

    static inline int
    Listen(uv_stream_t* server, uv_connection_cb on_connect, const int backlog=DEFAULT_SERVER_BACKLOG){
      return uv_listen(server, backlog, on_connect);
    }

    static inline int
    Accept(uv_stream_t* server, SessionBase* session){
      return uv_accept(server, session->GetStream());
    }

    static inline int
    ReadStart(SessionBase* session, uv_alloc_cb on_alloc, uv_read_cb on_read){
      return uv_read_start(session->GetStream(), on_alloc, on_read);
    }
   public://??????????
    virtual SessionType* CreateSession() const = 0;
   public:
    virtual ~ServerBase<SessionType>() = default;

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
      return (State)state_;
    }

    bool Run(const ServerPort& port){
      DLOG(INFO) << "starting the server....";
      SetState(ServerBase::State::kStartingState);
      VERIFY_UVRESULT2(FATAL, Bind(GetHandle(), port), "cannot bind server");
      VERIFY_UVRESULT2(FATAL, Listen(GetStream(), &OnNewConnection), "listen failure");

      SetState(ServerBase::State::kRunningState);
      DLOG(INFO) << "server listening on port: " << port;
      VERIFY_UVRESULT2(FATAL, uv_run(GetLoop(), UV_RUN_DEFAULT), "failed to run server loop");
      return true;
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name(){ return GetState() == ServerBase::State::k##Name##State; }
    FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };

  template<class Server>
  class ServerThread{
   protected:
    ThreadId thread_;

    static void HandleThread(uword param){
      auto instance = new Server();
      if(!instance->Run(Server::GetPort())){
        LOG(FATAL) << "cannot run the " << Server::GetName() << " loop on port: " << Server::GetPort();
      }
      pthread_exit(nullptr);
    }
   public:
    ServerThread() = default;
    ~ServerThread() = default;

    bool Start(){
      return platform::ThreadStart(&thread_, Server::GetName(), &HandleThread, (uword)0);
    }

    bool Join(){
      return platform::ThreadJoin(thread_);
    }
  };
}

#endif //TOKEN_SERVER_H