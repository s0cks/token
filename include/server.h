#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <uv.h>
#include <sstream>
#include "session.h"
#include "vthread.h"
#include "configuration.h"
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

  template<class M>
  class Server{
   private:
    typedef Server<M> ServerType;
    typedef std::shared_ptr<M> ServerMessagePtr;
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
        default:
          return stream << "Unknown";
      }
    }
   protected:
    const char* name_;
    ThreadId thread_;
    uv_loop_t* loop_;
    uv_tcp_t handle_;
    uv_async_t shutdown_;
    RelaxedAtomic<State> state_;

    Server(uv_loop_t* loop, const char* name):
      name_(strdup(name)),
      thread_(),
      loop_(loop),
      handle_(),
      shutdown_(),
      state_(Server::kStoppedState){
      shutdown_.data = this;
      handle_.data = this;
      CHECK_UVRESULT(uv_tcp_init(GetLoop(), &handle_), LOG_SERVER(ERROR), "cannot initialize the server handle");
    }

    void SetState(const State& state){
      state_ = state;
    }

    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
      buf->base = (char*)malloc(suggested_size);
      buf->len = suggested_size;
    }

    static void
    OnNewConnection(uv_stream_t* stream, int status){
      auto server = (ServerType*)stream->data;
      CHECK_UVRESULT(status, LOG_SERVER(ERROR), "connect failure");

      Session<M>* session = server->CreateSession();
      session->SetState(Session<M>::kConnectingState);
      CHECK_UVRESULT(Accept(stream, session), LOG_SERVER(ERROR), "accept failure");
      CHECK_UVRESULT(ReadStart(session, &AllocBuffer, &OnMessageReceived), LOG_SERVER(ERROR), "read failure");
    }

    static void
    OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
      auto session = (Session<M>*)stream->data;
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

      BufferPtr buffer = Buffer::From(buff->base, nread);
      do{
        ServerMessagePtr message = M::From(session, buffer);
        DLOG(INFO) << "received " << message->ToString() << " from " << session->GetUUID();
        session->OnMessageRead(message);
      } while(buffer->GetReadBytes() < buffer->GetBufferSize());
      free(buff->base);
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
    Accept(uv_stream_t* server, Session<M>* session){
      return uv_accept(server, session->GetStream());
    }

    static inline int
    ReadStart(Session<M>* session, uv_alloc_cb on_alloc, uv_read_cb on_read){
      return uv_read_start(session->GetStream(), on_alloc, on_read);
    }

    virtual Session<M>* CreateSession() const = 0;
   public:
    virtual ~Server(){
      if(name_)
        free((void*)name_);
    }

    const char* GetName() const{
      return name_;
    }

    virtual ServerPort GetPort() const{
      return 0;
    }

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
      return (State)state_;
    }

    bool Run(){
      SetState(Server::State::kStartingState);
      VERIFY_UVRESULT(Bind(GetHandle(), GetPort()), LOG_SERVER(ERROR), "cannot bind server");
      VERIFY_UVRESULT(Listen(GetStream(), &OnNewConnection), LOG_SERVER(ERROR), "listen failure");

      DLOG_THREAD(INFO) << "server listening on port: " << GetPort();
      SetState(Server::State::kRunningState);
      VERIFY_UVRESULT(uv_run(GetLoop(), UV_RUN_DEFAULT), LOG_SERVER(ERROR), "failed to run loop");
      return true;
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name(){ return GetState() == Server::State::k##Name##State; }
    FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };
}

#endif //TOKEN_SERVER_H