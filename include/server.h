#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#ifdef TOKEN_ENABLE_SERVER

#include <uv.h>
#include <sstream>
#include "vthread.h"
#include "configuration.h"
#include "atomic/relaxed_atomic.h"

#define DEFAULT_SERVER_BACKLOG 100

namespace Token{
#define FOR_EACH_SERVER_STATE(V) \
    V(Starting) \
    V(Running) \
    V(Stopping) \
    V(Stopped)

  typedef int32_t ServerPort;

  template<class M, class S>
  class Server{
   private:
    typedef Server<M, S> BaseType;
    typedef S SessionType;
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
    RelaxedAtomic<State> state_;

    Server(uv_loop_t* loop, const char* name):
      name_(strdup(name)),
      thread_(),
      loop_(loop),
      handle_(),
      state_(Server::kStoppedState){
      handle_.data = this;
      int err;
      if((err = uv_tcp_init(GetLoop(), GetHandle())) != 0){
        LOG(WARNING) << "rpc initialize error: " << uv_strerror(err);
        return;
      }
    }

    void SetState(const State& state){
      state_ = state;
    }

    static void
    HandleServerThread(uword parameter){
      Server<M, S>* server = (Server<M, S>*)parameter;
      server->SetState(Server::kStartingState);

      const char* name = server->GetName();
      int32_t port = server->GetPort();

      int err;
      if((err = Bind(server->GetHandle(), port)) != 0){
        LOG(WARNING) << name << " rpc bind failure: " << uv_strerror(err);
        server->SetState(Server::kStoppingState);
        goto exit;
      }

      if((err = Listen(server->GetStream(), &OnNewConnection)) != 0){
        LOG(WARNING) << name << " rpc listen failure: " << uv_strerror(err);
        server->SetState(Server::kStoppingState);
        goto exit;
      }

      LOG(INFO) << name << " rpc listening @" << port;
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
      BaseType* server = (BaseType*)stream->data;
      if(status != 0){
        LOG(ERROR) << "connection error: " << uv_strerror(status);
        return;
      }

      S* session = server->CreateSession();
      session->SetState(SessionType::kConnectingState);

      int err;
      if((err = Accept(stream, session)) != 0){
        LOG(WARNING) << "rpc accept failure: " << uv_strerror(err);
        return; //TODO: session->Disconnect();
      }

      if((err = ReadStart(session, &AllocBuffer, &OnMessageReceived)) != 0){
        LOG(WARNING) << "rpc read failure: " << uv_strerror(err);
        return; //TODO: session->Disconnect();
      }
    }

    static void
    OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
      SessionType* session = (SessionType*)stream->data;
      if(nread == UV_EOF){
        LOG(ERROR) << "client disconnected!";
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
        session->OnMessageRead(message);
      } while(buffer->GetReadBytes() < buffer->GetBufferSize());
      free(buff->base);
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
    Accept(uv_stream_t* server, SessionType* session){
      return uv_accept(server, session->GetStream());
    }

    static inline int
    ReadStart(SessionType* session, uv_alloc_cb on_alloc, uv_read_cb on_read){
      return uv_read_start(session->GetStream(), on_alloc, on_read);
    }

    bool StartThread(){
      return Thread::StartThread(&thread_, name_, &HandleServerThread, (uword)this);
    }

    bool JoinThread(){
      return Thread::StopThread(thread_);
    }

    virtual SessionType* CreateSession() const = 0;
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

#endif//TOKEN_ENABLE_SERVER
#endif //TOKEN_SERVER_H