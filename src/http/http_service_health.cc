#include "http/http_service_health.h"

namespace token{
  HttpHealthService* HttpHealthService::GetInstance(){
    static HttpHealthService instance;
    return &instance;
  }

  HttpHealthService::HttpHealthService(uv_loop_t* loop):
    HttpService(loop, GetThreadName()),
    health_(std::make_shared<HealthController>()){
    if(!GetHealthController()->Initialize(&router_))
      LOG(WARNING) << "cannot initialize HealthController";
  }

  static ThreadId thread_;

  bool HttpHealthServiceThread::Join(){
    return ThreadJoin(thread_);
  }

  bool HttpHealthServiceThread::Start(){
    return ThreadStart(&thread_, HttpHealthService::GetThreadName(), &HandleThread, (uword)HttpHealthService::GetInstance());
  }

  void HttpHealthServiceThread::HandleThread(uword param){
    auto instance = (HttpHealthService*)param;
    DLOG_THREAD_IF(ERROR, !instance->Run()) << "Failed to run loop";
    pthread_exit(nullptr);
  }
}