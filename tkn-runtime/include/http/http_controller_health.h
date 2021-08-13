#ifndef TOKEN_HTTP_CONTROLLER_HEALTH_H
#define TOKEN_HTTP_CONTROLLER_HEALTH_H

#include "http/http_controller.h"

namespace token{
  namespace http{
    class HealthController;
    typedef std::shared_ptr<HealthController> HealthControllerPtr;

#define FOR_EACH_HEALTH_CONTROLLER_ENDPOINT(V) \
    V(GET, "/ready", GetReadyStatus)           \
    V(GET, "/live", GetLiveStatus)

    class HealthController : public Controller{
     public:
      HealthController() = default;
      ~HealthController() override = default;

#define DECLARE_ENDPOINT(Method, Path, Name) \
    HTTP_CONTROLLER_ENDPOINT(Name);

      FOR_EACH_HEALTH_CONTROLLER_ENDPOINT(DECLARE_ENDPOINT)
#undef DECLARE_ENDPOINT

      bool Initialize(Router& router) override{
#define REGISTER_ENDPOINT(Method, Path, Name) \
        HTTP_CONTROLLER_##Method(Path, Name);

        FOR_EACH_HEALTH_CONTROLLER_ENDPOINT(REGISTER_ENDPOINT)
#undef REGISTER_ENDPOINT
        return true;
      }

      static inline HealthControllerPtr
      NewInstance(){
        return std::make_shared<HealthController>();
      }
    };

    static inline HealthControllerPtr
    NewHealthController(){
      return std::make_shared<HealthController>();
    }
  }
}

#endif//TOKEN_HTTP_CONTROLLER_HEALTH_H