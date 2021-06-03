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

      static inline bool
      IsEnabled(){
        return IsValidPort(HealthService::GetPort());
      }

      static inline ServerPort
      GetPort(){
        return FLAGS_healthcheck_port;
      }

      static inline const char*
      GetName(){
        return "http/health";
      }

      static inline HealthServicePtr
      NewInstance(){
        return std::make_shared<HealthService>();
      }
    };

    class HealthServiceThread : public ServiceThread<HealthService>{
     public:
      HealthServiceThread():
        ServiceThread<HealthService>(){}
      HealthServiceThread(const HealthServiceThread& other) = default;
      ~HealthServiceThread() = default;

      HealthServiceThread& operator=(const HealthServiceThread& other) = default;
    };
  }
}

#endif//TOKEN_HTTP_SERVICE_HEALTH_H