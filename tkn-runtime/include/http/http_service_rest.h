#ifndef TOKEN_HTTP_SERVICE_REST_H
#define TOKEN_HTTP_SERVICE_REST_H

#include "http/http_service.h"

#include "http/http_controller_pool.h"

namespace token{
  namespace http{
    class RestService;
    typedef std::shared_ptr<RestService> RestServicePtr;

    class RestService : public ServiceBase{
     private:
      PoolControllerPtr pool_;
     public:
      RestService(uv_loop_t* loop,
                  const ObjectPoolPtr& pool);
      ~RestService() override = default;

      PoolControllerPtr GetPoolController() const{
        return pool_;
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

      static RestServicePtr NewInstance();
      static RestServicePtr NewInstance(uv_loop_t* loop, const ObjectPoolPtr& pool);

      static inline RestServicePtr
      NewInstance(const BlockChainPtr& chain, const ObjectPoolPtr& pool){
        return NewInstance(uv_loop_new(), pool);
      }
    };

    class RestServiceThread : public ServiceThread<RestService>{};
  }
}

#endif//TOKEN_HTTP_SERVICE_REST_H