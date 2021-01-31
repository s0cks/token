#ifndef TOKEN_HTTP_CONTROLLER_HEALTH_H
#define TOKEN_HTTP_CONTROLLER_HEALTH_H

#ifdef TOKEN_ENABLE_HEALTH_SERVICE

#include "http/http_service.h"
#include "http/http_controller.h"

namespace Token{
  class HealthController : HttpController{
   private:
    HealthController() = delete;

    HTTP_CONTROLLER_ENDPOINT(GetReadyStatus);
    HTTP_CONTROLLER_ENDPOINT(GetLiveStatus);
   public:
    ~HealthController() = delete;

    HTTP_CONTROLLER_INIT(){
      HTTP_CONTROLLER_GET("/ready", GetReadyStatus);
      HTTP_CONTROLLER_GET("/live", GetLiveStatus);
    }
  };
}

#endif//TOKEN_ENABLE_HEALTH_SERVICE
#endif//TOKEN_HTTP_CONTROLLER_HEALTH_H