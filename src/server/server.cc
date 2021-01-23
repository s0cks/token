#ifdef TOKEN_ENABLE_SERVER

#include <algorithm>
#include <random>
#include <condition_variable>

#include "pool.h"
#include "utils/tcp.h"
#include "configuration.h"
#include "server/server.h"
#include "utils/crash_report.h"
#include "server/server_session.h"
#include "unclaimed_transaction.h"
#include "peer/peer_session_manager.h"

namespace Token{
  static pthread_t thread_;
  static uv_tcp_t* handle_ = new uv_tcp_t();
  static uv_async_t shutdown_;

  static std::mutex mutex_;
  static std::condition_variable cond_;
  static Server::State state_ = Server::State::kStopped;
  static Server::Status status_ = Server::Status::kOk;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

  uv_tcp_t* Server::GetHandle(){
    return handle_;
  }

  void Server::WaitForState(Server::State state){
    LOCK;
    while(state_ != state) WAIT;
    UNLOCK;
  }

  void Server::SetState(Server::State state){
    LOCK;
    state_ = state;
    UNLOCK;
    SIGNAL_ALL;
  }

  Server::State Server::GetState(){
    LOCK_GUARD;
    return state_;
  }

  void Server::SetStatus(Server::Status status){
    LOCK;
    status_ = status;
    UNLOCK;
    SIGNAL_ALL;
  }

  Server::Status Server::GetStatus(){
    LOCK_GUARD;
    return status_;
  }

  void Server::OnWalk(uv_handle_t* handle, void* data){
    uv_close(handle, &OnClose);
  }

  void Server::OnClose(uv_handle_t* handle){}

  void Server::HandleTerminateCallback(uv_async_t* handle){
    SetState(Server::kStopping);
    uv_stop(handle->loop);

    int err;
    if((err = uv_loop_close(handle->loop)) == UV_EBUSY){
      uv_walk(handle->loop, &OnWalk, NULL);
    }

    uv_run(handle->loop, UV_RUN_DEFAULT);
    if((err = uv_loop_close(handle->loop))){
      LOG(ERROR) << "failed to close server loop: " << uv_strerror(err);
    } else{
      LOG(INFO) << "server loop closed.";
    }
    SetState(Server::kStopped);
  }

  bool Server::Start(){
    if(!IsStopped()){
      LOG(WARNING) << "the server is already running.";
      return false;
    }
    return Thread::StartThread(&thread_, "server", &HandleServerThread, 0);
  }

  bool Server::Stop(){
    if(!IsRunning()){
      return true;
    } // should we return false?
    uv_async_send(&shutdown_);
    return Thread::StopThread(thread_);
  }

  void Server::HandleServerThread(uword parameter){
    LOG(INFO) << "starting server...";
    SetState(State::kStarting);
    uv_loop_t* loop = uv_loop_new();
    uv_tcp_init(loop, GetHandle());
    uv_tcp_keepalive(GetHandle(), 1, 60);

    uv_async_init(loop, &shutdown_, &HandleTerminateCallback);

    if(!ServerBind(GetHandle(), FLAGS_server_port)){
      SetState(State::kStopping);
      goto exit;
    }

    if(!ServerListen((uv_stream_t*) GetHandle(), &OnNewConnection)){
      SetState(State::kStopping);
      goto exit;
    }

    SetState(State::kRunning);

    LOG(INFO) << "server " << GetID() << " listening @" << FLAGS_server_port;
    uv_run(loop, UV_RUN_DEFAULT);
  exit:
    SetState(State::kStopped);
    pthread_exit(0);
  }

  void Server::OnNewConnection(uv_stream_t* stream, int status){
    if(status != 0){
      LOG(ERROR) << "connection error: " << uv_strerror(status);
      return;
    }

    ServerSession* session = ServerSession::NewInstance(stream->loop);
    session->SetState(Session::kConnecting);
    LOG(INFO) << "client is connecting...";
    if(!ServerAccept(stream, session->GetStream())){
      //TODO: session->Disconnect();
      return;
    }

    LOG(INFO) << "client connected";
    if(!ServerReadStart(session->GetStream(), &ServerSession::AllocBuffer, &OnMessageReceived)){
      //TODO: session->Disconnect();
      return;
    }
  }

  void Server::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
    ServerSession* session = (ServerSession*) stream->data;
    if(nread == UV_EOF){
      LOG(ERROR) << "client disconnected!";
      return;
    } else if(nread < 0){
      LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
      return;
    } else if(nread == 0){
      LOG(WARNING) << "zero message size received";
      return;
    } else if(nread > Session::kBufferSize){
      LOG(ERROR) << "too large of a buffer: " << nread;
      return;
    }

    int64_t total_bytes = static_cast<int64_t>(nread);
    MessageBufferReader reader(buff, total_bytes);
    while(reader.HasNext()){
      MessagePtr next = reader.Next();
      LOG(INFO) << "next: " << next->ToString();
      switch(next->GetMessageType()){
#define DEFINE_HANDLER_CASE(Name) \
        case Message::k##Name##MessageType: \
            session->Handle##Name##Message(session, std::static_pointer_cast<Name##Message>(next)); \
            break;
        FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE)
#undef DEFINE_HANDLER_CASE
        case Message::kUnknownMessageType:
        default: //TODO: handle properly
          break;
      }
    }
    free(buff->base);
  }
}

#endif//TOKEN_ENABLE_SERVER