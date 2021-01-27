#ifdef TOKEN_ENABLE_HEALTH_SERVICE

#include <mutex>
#include <condition_variable>
#include "configuration.h"
#include "http/controller.h"
#include "http/health/health_service.h"

namespace Token{
  static ThreadId thread_;
  static std::atomic<HealthCheckService::State> state_ = {HealthCheckService::kStopped};
  static std::atomic<HealthCheckService::Status> status_ = {HealthCheckService::kOk};
  static HttpRouter* router_ = nullptr;
  static uv_tcp_t* server_ = nullptr;
  static uv_async_t* shutdown_ = nullptr;

  HealthCheckService::State HealthCheckService::GetState(){
    return state_.load(std::memory_order_seq_cst);
  }

  void HealthCheckService::SetState(State state){
    state_.store(state, std::memory_order_seq_cst);
  }

  HealthCheckService::Status HealthCheckService::GetStatus(){
    return status_.load(std::memory_order_seq_cst);
  }

  void HealthCheckService::SetStatus(Status status){
    status_.store(status, std::memory_order_seq_cst);
  }

  void HealthCheckService::WaitForState(State state){
    //TODO: implement
  }

  bool HealthCheckService::Start(){
    if(!IsStopped()){
      LOG(WARNING) << "the health check service is already running.";
      return false;
    }

    LOG(INFO) << "initializing the health check service....";
    router_ = new HttpRouter();
    server_ = new uv_tcp_t();
    shutdown_ = new uv_async_t();
    HealthController::Initialize(router_);
    return Thread::StartThread(&thread_, "health-svc", &HandleServiceThread, 0);
  }

  bool HealthCheckService::Stop(){
    if(!IsRunning()){
      return true;
    } // should we return false?
    uv_async_send(shutdown_);
    return Thread::StopThread(thread_);
  }

  void HealthCheckService::HandleServiceThread(uword parameter){
    SetState(HealthCheckService::kStarting);
    uv_loop_t* loop = uv_loop_new();
    uv_async_init(loop, shutdown_, &OnShutdown);

    uv_tcp_init(loop, server_);
    int32_t port = FLAGS_healthcheck_port;
    if(!Bind(server_, port)){
      LOG(WARNING) << "couldn't bind health check service on port: " << port;
      goto exit;
    }

    int result;
    if((result = uv_listen((uv_stream_t*) server_, 100, &OnNewConnection)) != 0){
      LOG(WARNING) << "health check service couldn't listen on port " << port << ": " << uv_strerror(result);
      goto exit;
    }

    LOG(INFO) << "health check service listening @" << port;
    SetState(State::kRunning);
    uv_run(loop, UV_RUN_DEFAULT);
    exit:
    uv_loop_close(loop);
    pthread_exit(nullptr);
  }

  void HealthCheckService::OnShutdown(uv_async_t* handle){
    SetState(HealthCheckService::kStopping);
    uv_stop(handle->loop);
    if(!HttpService::ShutdownService(handle->loop))
      LOG(WARNING) << "couldn't shutdown the HealthCheck service";
  }

  void HealthCheckService::OnNewConnection(uv_stream_t* stream, int status){
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

  void HealthCheckService::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
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
    HttpRouterMatch match = router_->Find(request);
    if(match.IsNotFound()){
      std::stringstream ss;
      ss << "Cannot find: " << request->GetPath();
      return session->Send(NewNotFoundResponse(session, ss));
    } else if(match.IsMethodNotSupported()){
      std::stringstream ss;
      ss << "Method Not Supported for: " << request->GetPath();
      return session->Send(NewNotSupportedResponse(session, ss));
    } else{
      assert(match.IsOk());

      // weirdness :(
      request->SetParameters(match.GetParameters());
      HttpRouteHandler& handler = match.GetHandler();
      handler(session, request);
    }
  }

  void HealthController::HandleGetReadyStatus(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewOkResponse(session));
  }

  void HealthController::HandleGetLiveStatus(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewOkResponse(session));
  }
}

#endif//TOKEN_ENABLE_HEALTH_SERVICE