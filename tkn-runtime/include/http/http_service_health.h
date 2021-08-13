#ifndef TOKEN_HTTP_SERVICE_HEALTH_H
#define TOKEN_HTTP_SERVICE_HEALTH_H

#include "flags.h"
#include "http/http_service.h"
#include "http/http_controller_health.h"

namespace token{
  class Runtime;
  namespace http{
    class HealthService : public ServiceBase{
    protected:
      HealthController controller_health_;
    public:
      explicit HealthService(Runtime* runtime);
      ~HealthService() override = default;

      HealthController& GetHealthController(){
        return controller_health_;
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
    };
  }
}

#endif//TOKEN_HTTP_SERVICE_HEALTH_H