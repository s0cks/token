#ifndef TOKEN_HEALTH_CONTROLLER_H
#define TOKEN_HEALTH_CONTROLLER_H

#ifdef TOKEN_ENABLE_HEALTH_SERVICE

#include "http/service.h"
#include "http/controller.h"

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
#endif//TOKEN_HEALTH_CONTROLLER_H