#ifndef TOKEN_HTTP_SERVICE_REST_H
#define TOKEN_HTTP_SERVICE_REST_H

#include "http/http_service.h"

#include "http/http_controller_pool.h"

namespace token{
  class Runtime;
  namespace http{
    class RestService : public ServiceBase{
     private:
      PoolController controller_pool_;
     public:
      explicit RestService(Runtime* runtime);
      ~RestService() override = default;

      PoolController& GetPoolController(){
        return controller_pool_;
      }

      static inline bool
      IsEnabled(){
        return IsValidPort(RestService::GetPort());
      }

      static inline ServerPort
      GetPort(){
        return FLAGS_service_port;
      }

      static inline const char*
      GetName(){
        return "http/rest";
      }
    };
  }
}

#endif//TOKEN_HTTP_SERVICE_REST_H