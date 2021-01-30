#ifdef TOKEN_ENABLE_SERVER

#include "configuration.h"
#include "peer/peer_session_manager.h"

#include "server/server.h"
#include "server/session.h"
#include "atomic/relaxed_atomic.h"

namespace Token{
  bool Server::StartThread(){
    if(IsRunning())
      return false;
    return Thread::StartThread(&thread_, "server", &HandleServerThread, (uword)this);
  }

  void Server::HandleServerThread(uword parameter){
    Server* server = (Server*)parameter;
    server->SetState(Server::kStartingState);

    uv_loop_t* loop = uv_loop_new();

    int err;
    if((err = Bind(server->GetHandle(), FLAGS_server_port)) != 0){
      LOG(WARNING) << "server bind failure: " << uv_strerror(err);
      server->SetState(Server::kStoppingState);
      goto exit;
    }

    if((err = Listen(server->GetStream(), &OnNewConnection)) != 0){
      LOG(WARNING) << "server listen failure: " << uv_strerror(err);
      server->SetState(Server::kStoppingState);
      goto exit;
    }

    LOG(INFO) << "server listening @" << FLAGS_server_port;
    server->SetState(State::kRunningState);
    uv_run(loop, UV_RUN_DEFAULT);
  exit:
    server->SetState(State::kStoppedState);
    pthread_exit(0);
  }

  void Server::OnNewConnection(uv_stream_t* stream, int status){
    if(status != 0){
      LOG(ERROR) << "connection error: " << uv_strerror(status);
      return;
    }

    ServerSession* session = new ServerSession(stream->loop);
    session->SetState(Session::kConnectingState);

    int err;
    if((err = Accept(stream, session->GetChannel())) != 0){
      LOG(WARNING) << "server accept failure: " << uv_strerror(err);
      return; //TODO: session->Disconnect();
    }

    if((err = ReadStart(session->GetChannel(), &Session::AllocBuffer, &OnMessageReceived)) != 0){
      LOG(WARNING) << "server read failure: " << uv_strerror(err);
      return; //TODO: session->Disconnect();
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
    } else if(nread > 65536){
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