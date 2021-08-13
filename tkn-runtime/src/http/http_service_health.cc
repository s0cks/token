#include "runtime.h"
#include "http/http_service_health.h"

namespace token{
  namespace http{
    HealthService::HealthService(Runtime* runtime):
      ServiceBase(runtime->loop()),
      controller_health_(){
      controller_health_.Initialize(router_);
    }
  }
}