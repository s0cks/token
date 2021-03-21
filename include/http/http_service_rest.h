#ifndef TOKEN_HTTP_SERVICE_REST_H
#define TOKEN_HTTP_SERVICE_REST_H

#include "http/http_service.h"

#include "http/http_controller_pool.h"
#include "http/http_controller_chain.h"
#include "http/http_controller_wallet.h"

namespace token{
  class HttpRestService : public HttpService{
   private:
    std::shared_ptr<PoolController> pool_;
    std::shared_ptr<ChainController> chain_;
    std::shared_ptr<WalletController> wallet_;
   public:
    explicit HttpRestService(uv_loop_t* loop=uv_loop_new());
    ~HttpRestService() override = default;

    std::shared_ptr<PoolController> GetPoolController() const{
      return pool_;
    }

    std::shared_ptr<ChainController> GetChainController() const{
      return chain_;
    }

    std::shared_ptr<WalletController> GetWalletController() const{
      return wallet_;
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

    static HttpRestService* GetInstance();
  };

  class HttpRestServiceThread{
   protected:
    static void HandleThread(uword param);
   public:
    HttpRestServiceThread() = delete;
    ~HttpRestServiceThread() = delete;
    static bool Join();
    static bool Start();
  };
}

#endif//TOKEN_HTTP_SERVICE_REST_H