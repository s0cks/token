#include "http/http_controller_health.h"

namespace token{
#define DEFINE_ENDPOINT_HANDLER(Method, Path, Name) \
  HTTP_CONTROLLER_ROUTE_HANDLER(HealthController, Name);

  FOR_EACH_HEALTH_CONTROLLER_ENDPOINT(DEFINE_ENDPOINT_HANDLER)
#undef DEFINE_ENDPOINT_HANDLER

  HTTP_CONTROLLER_ENDPOINT_HANDLER(HealthController, GetReadyStatus){
    return session->Send(NewOkResponse(session));
  }

  HTTP_CONTROLLER_ENDPOINT_HANDLER(HealthController, GetLiveStatus){
    return session->Send(NewOkResponse(session));
  }
}