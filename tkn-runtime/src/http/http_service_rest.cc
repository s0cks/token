#include "runtime.h"
#include "http/http_service_rest.h"

namespace token{
  namespace http{
    RestService::RestService(Runtime* runtime):
      ServiceBase(runtime->loop()),
      controller_pool_(runtime->GetPool()){
      controller_pool_.Initialize(router_);
    }
  }
}