#include "http/http_service_health.h"

namespace token{
  static HttpHealthService instance;

  HttpHealthService::HttpHealthService(uv_loop_t* loop):
    HttpService(loop),
    health_(std::make_shared<HealthController>()){
    if(!GetHealthController()->Initialize(&router_))
      LOG(WARNING) << "cannot initialize HealthController";
  }

  bool HttpHealthService::Start(){
    return instance.StartThread();
  }

  bool HttpHealthService::Shutdown(){
    LOG(WARNING) << "HttpHealthService::Shutdown() not implemented.";
    return false; //TODO: implement HttpRestService::Shutdown()
  }

  bool HttpHealthService::WaitForShutdown(){
    return instance.JoinThread();
  }

#define DEFINE_STATE_CHECK(Name) \
  bool HttpHealthService::IsService##Name(){ return instance.Is##Name(); }
  FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
}