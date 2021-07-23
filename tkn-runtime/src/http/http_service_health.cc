#include "http/http_service_health.h"

namespace token{
  namespace http{
    HealthService::HealthService(uv_loop_t* loop):
      ServiceBase(loop),
      controller_(NewHealthController()){
      if(!GetHealthController()->Initialize(GetRouter()))
        LOG(WARNING) << "cannot initialize HealthController";
    }

    HealthServicePtr HealthService::NewInstance(){
      return std::unique_ptr<HealthService>(new HealthService());
    }
  }
}