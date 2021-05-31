#ifndef TOKEN_HTTP_SERVICE_REST_H
#define TOKEN_HTTP_SERVICE_REST_H

#include "http/http_service.h"
#include "http/http_controller_pool.h"
#include "http/http_controller_chain.h"
#include "http/http_controller_wallet.h"

namespace token{
  namespace http{
    class RestService;
    typedef std::shared_ptr<RestService> RestServicePtr;

    class RestService : public ServiceBase{
     private:
      PoolControllerPtr pool_;
      ChainControllerPtr chain_;
      WalletControllerPtr wallets_;
     public:
      explicit RestService(uv_loop_t* loop=uv_loop_new());
      ~RestService() override = default;

      PoolControllerPtr GetPoolController() const{
        return pool_;
      }

      ChainControllerPtr GetChainController() const{
        return chain_;
      }

      WalletControllerPtr GetWalletController() const{
        return wallets_;
      }

      ServerPort GetPort() const override{
        return GetServerPort();
      }

      static inline ServerPort
      GetServerPort(){
        return FLAGS_service_port;
      }

      static inline const char*
      GetThreadName(){
        return "http/rest";
      }

      static inline RestServicePtr
      NewInstance(){
        return std::make_shared<RestService>();
      }
    };

    class RestServiceThread : public ServiceThread<RestService>{};
  }
}

#endif//TOKEN_HTTP_SERVICE_REST_H