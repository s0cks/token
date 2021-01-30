#ifdef TOKEN_ENABLE_REST_SERVICE

#include "atomic/relaxed_atomic.h"
#include "server/http/rest/rest_service.h"
#include "server/http/rest/chain_controller.h"
#include "server/http/rest/pool_controller.h"
#include "server/http/rest/wallet_controller.h"

namespace Token{
  static pthread_t thread_;
  static RelaxedAtomic<RestService::State> state_ = { RestService::kStopped };
  static RelaxedAtomic<RestService::Status> status_ = { RestService::kOk };
  static HttpRouter router_;
  static uv_async_t shutdown_;

  RestService::State RestService::GetState(){
    return state_;
  }

  void RestService::SetState(State state){
    state_ = state;
  }

  RestService::Status RestService::GetStatus(){
    return status_;
  }

  void RestService::SetStatus(Status status){
    status_ = status;
  }

  bool RestService::StartThread(){
    if(!IsStopped()){
      LOG(WARNING) << "the rest service is already running.";
      return false;
    }

    LOG(INFO) << "starting the rest service....";
    ObjectPoolController::Initialize(&router_);
    WalletController::Initialize(&router_);
    BlockChainController::Initialize(&router_);
    return Thread::StartThread(&thread_, "rest-svc", &HandleServiceThread, 0);
  }

  void RestService::OnShutdown(uv_async_t* handle){
    SetState(RestService::kStopping);
    if(!HttpService::ShutdownService(handle->loop)){
      LOG(WARNING) << "couldn't shutdown the rest service.";
    }
    SetState(RestService::kStopped);
  }

  bool RestService::SendShutdown(){
    if(!IsRunning()){
      LOG(WARNING) << "rest service isn't running.";
      return true;
    }

    int err;
    if((err = uv_async_send(&shutdown_)) != 0){
      LOG(ERROR) << "cannot invoke send shutdown notice to rest service event loop: " << uv_strerror(err);
      return false;
    }
    return true;
  }

  bool RestService::JoinThread(){
    return Thread::StopThread(thread_);
  }

  void RestService::HandleServiceThread(uword parameter){
    SetState(RestService::kStarting);
    uv_loop_t* loop = uv_loop_new();
    uv_async_init(loop, &shutdown_, &OnShutdown);

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    int32_t port = FLAGS_service_port;
    if(!Bind(&server, port)){
      LOG(WARNING) << "couldn't bind the rest service on port: " << port;
      goto exit;
    }

    int result;
    if((result = uv_listen((uv_stream_t*) &server, 100, &OnNewConnection)) != 0){
      LOG(WARNING) << "the rest service couldn't listen on port " << port << ": " << uv_strerror(result);
      goto exit;
    }

    LOG(INFO) << "rest service listening @" << port;
    SetState(State::kRunning);
    uv_run(loop, UV_RUN_DEFAULT);
    LOG(INFO) << "the rest service has stopped.";
    exit:
    pthread_exit(nullptr);
  }

  void RestService::OnNewConnection(uv_stream_t* stream, int status){
    HttpSession* session = new HttpSession(stream);
    if(!Accept(stream, session)){
      LOG(WARNING) << "couldn't accept new connection.";
      return;
    }

    int result;
    if((result = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
      LOG(WARNING) << "server read failure: " << uv_strerror(result);
      return;
    }
  }

  void RestService::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
    HttpSession* session = (HttpSession*) stream->data;
    if(nread == UV_EOF){
      return;
    }
    if(nread < 0){
      LOG(WARNING) << "server read failure: " << uv_strerror(nread);
      session->Close();
      return;
    }

    HttpRequestPtr request = HttpRequest::NewInstance(session, buff->base, buff->len);
    HttpRouterMatch match = router_.Find(request);
    if(match.IsNotFound()){
      std::stringstream ss;
      ss << "Cannot find: " << request->GetPath();
      return session->Send(NewNotFoundResponse(session, ss));
    } else if(match.IsNotSupported()){
      std::stringstream ss;
      ss << "Method Not Supported for: " << request->GetPath();
      return session->Send(NewNotSupportedResponse(session, ss));
    } else{
      assert(match.IsOk());

      // weirdness :(
      request->SetPathParameters(match.GetPathParameters());
      request->SetQueryParameters(match.GetQueryParameters());
      HttpRouteHandler handler = match.GetHandler();
      handler(session, request);
    }
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE