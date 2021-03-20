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

#define SERVER_LOG(LevelName) \
  LOG(LevelName) << "[server] "
#define DLOG_SERVER(LevelName) \
  DLOG(LevelName) << "[server] "

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

      int err;
      if((err = uv_tcp_init(loop_, &handle_)) != 0){
        THREAD_LOG(ERROR) << "cannot initialize server handle: " << uv_strerror(err);
        return;
      }
    }

    void SetState(const State& state){
      state_ = state;
    }

    static void
    HandleServerThread(uword parameter){
      auto server = (ServerType*)parameter;
      server->SetState(Server::kStartingState);

      int32_t port = server->GetPort();

      int err;
      if((err = Bind(server->GetHandle(), port)) != 0){
        SERVER_LOG(WARNING) << "bind failure: " << uv_strerror(err);
        server->SetState(Server::kStoppingState);
        goto exit;
      }

      if((err = Listen(server->GetStream(), &OnNewConnection)) != 0){
        SERVER_LOG(WARNING) "listen failure: " << uv_strerror(err);
        server->SetState(Server::kStoppingState);
        goto exit;
      }

      SERVER_LOG(INFO) << "server listening @" << port;
      server->SetState(State::kRunningState);
      uv_run(server->GetLoop(), UV_RUN_DEFAULT);
    exit:
      server->SetState(State::kStoppedState);
      pthread_exit(0);
    }

    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
      buf->base = (char*)malloc(suggested_size);
      buf->len = suggested_size;
    }

    static void
    OnNewConnection(uv_stream_t* stream, int status){
      ServerType* server = (ServerType*)stream->data;
      if(status != 0){
        SERVER_LOG(ERROR) << "connection error: " << uv_strerror(status);
        return;
      }

      Session<M>* session = server->CreateSession();
      session->SetState(Session<M>::kConnectingState);

      int err;
      if((err = Accept(stream, session)) != 0){
        SERVER_LOG(ERROR) << "accept failure: " << uv_strerror(err);
        return; //TODO: session->Disconnect();
      }

      if((err = ReadStart(session, &AllocBuffer, &OnMessageReceived)) != 0){
        SERVER_LOG(ERROR) << "read failure: " << uv_strerror(err);
        return; //TODO: session->Disconnect();
      }
    }

    static void
    OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
      Session<M>* session = (Session<M>*)stream->data;
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
      uv_walk(handle->loop, &OnWalk, NULL);
    }

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
    Accept(uv_stream_t* server, Session<M>* session){
      return uv_accept(server, session->GetStream());
    }

    static inline int
    ReadStart(Session<M>* session, uv_alloc_cb on_alloc, uv_read_cb on_read){
      return uv_read_start(session->GetStream(), on_alloc, on_read);
    }

    bool StartThread(){
      return ThreadStart(&thread_, name_, &HandleServerThread, (uword)this);
    }

    bool JoinThread(){
      return ThreadJoin(thread_);
    }

    bool SendShutdown(){
      int err;
      if((err = uv_async_send(&shutdown_)) != 0){
        THREAD_LOG(ERROR) << "cannot send shutdown: " << uv_strerror(err);
        return false;
      }
      return true;
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
      return state_;
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name(){ return GetState() == Server::State::k##Name##State; }
    FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };
}

#endif //TOKEN_SERVER_H