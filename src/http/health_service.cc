#ifdef TOKEN_ENABLE_HEALTH_SERVICE

#include "http/health_service.h"

#include "http/controller/health_controller.h"

namespace Token{
  static HttpHealthService instance;

  HttpHealthService::HttpHealthService(uv_loop_t* loop):
    HttpService(loop){
    HealthController::Initialize(&router_);
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

  HttpRouter* HttpHealthService::GetServiceRouter(){
    return instance.GetRouter();
  }

#define DEFINE_STATE_CHECK(Name) \
  bool HttpHealthService::IsService##Name(){ return instance.Is##Name(); }
  FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
}

#endif//TOKEN_ENABLE_HEALTH_SERVICE