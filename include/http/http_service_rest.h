#ifndef TOKEN_HTTP_SERVICE_REST_H
#define TOKEN_HTTP_SERVICE_REST_H

#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/http_service.h"

#include "http/http_controller_pool.h"
#include "http/http_controller_chain.h"
#include "http/http_controller_wallet.h"

namespace token{
  class HttpRestService : HttpService{
   private:
    std::shared_ptr<PoolController> pool_;
    std::shared_ptr<ChainController> chain_;
    std::shared_ptr<WalletController> wallet_;
   public:
    HttpRestService(uv_loop_t* loop=uv_loop_new());
    ~HttpRestService() = default;

    std::shared_ptr<PoolController> GetPoolController() const{
      return pool_;
    }

    std::shared_ptr<ChainController> GetChainController() const{
      return chain_;
    }

    std::shared_ptr<WalletController> GetWalletController() const{
      return wallet_;
    }

    ServerPort GetPort() const{
      return FLAGS_service_port;
    }

    static bool Start();
    static bool Shutdown();
    static bool WaitForShutdown();

#define DECLARE_STATE_CHECK(Name) \
    static bool IsService##Name();
    FOR_EACH_SERVER_STATE(DECLARE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };
}

#endif//TOKEN_ENABLE_REST_SERVICE
#endif//TOKEN_HTTP_SERVICE_REST_H