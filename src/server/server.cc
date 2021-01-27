#ifdef TOKEN_ENABLE_SERVER

#include <algorithm>
#include <random>
#include <condition_variable>

#include "pool.h"
#include "utils/tcp.h"
#include "configuration.h"
#include "server/server.h"
#include "utils/crash_report.h"
#include "utils/relaxed_atomic.h"
#include "server/server_session.h"
#include "peer/peer_session_manager.h"

namespace Token{
  static pthread_t thread_;
  static uv_tcp_t* handle_ = new uv_tcp_t(); // TODO: don't alloc handle_
  static uv_async_t shutdown_;
  static RelaxedAtomic<Server::State> state_ = { Server::kStopped };
  static RelaxedAtomic<Server::Status> status_ = { Server::kOk };

  uv_tcp_t* Server::GetHandle(){
    return handle_;
  }

  void Server::SetState(const State& state){
    state_ = state;
  }

  Server::State Server::GetState(){
    return state_;
  }

  void Server::SetStatus(const Status& status){
    status_ = status;
  }

  Server::Status Server::GetStatus(){
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

  bool Server::StartThread(){
    if(!IsStopped()){
      LOG(WARNING) << "the server is already running.";
      return false;
    }

    return Thread::StartThread(&thread_, "server", &HandleServerThread, 0);
  }

  bool Server::SendShutdown(){
    if(!IsRunning()){
      LOG(WARNING) << "server is not running.";
      return true;
    }

    int err;
    if((err = uv_async_send(&shutdown_)) != 0){
      LOG(ERROR) << "cannot invoke send shutdown notice to server event loop: " << uv_strerror(err);
      return false;
    }
    return true;
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

  bool Server::JoinThread(){
    return Thread::StopThread(thread_);
  }
}

#endif//TOKEN_ENABLE_SERVER