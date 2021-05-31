#ifndef TOKEN_HTTP_SERVICE_HEALTH_H
#define TOKEN_HTTP_SERVICE_HEALTH_H

#include "http/http_service.h"
#include "http/http_controller_health.h"

namespace token{
  namespace http{
    class HealthService;
    typedef std::shared_ptr<HealthService> HealthServicePtr;

    class HealthService : public ServiceBase{
     protected:
      HealthControllerPtr controller_;
     public:
      explicit HealthService(uv_loop_t* loop=uv_loop_new());
      ~HealthService() override = default;

      HealthControllerPtr GetHealthController() const{
        return controller_;
      }

      ServerPort GetPort() const override{
        return GetServerPort();
      }

      static inline const char*
      GetThreadName(){
        return "http/health";
      }

      static inline ServerPort
      GetServerPort(){
        return FLAGS_healthcheck_port;
      }

      static inline HealthServicePtr
      NewInstance(){
        return std::make_shared<HealthService>();
      }
    };

    class HealthServiceThread : public ServiceThread<HealthService>{};
  }
}

#endif//TOKEN_HTTP_SERVICE_HEALTH_H