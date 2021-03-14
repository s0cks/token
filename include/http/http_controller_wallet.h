#ifndef TOKEN_HTTP_CONTROLLER_WALLET_H
#define TOKEN_HTTP_CONTROLLER_WALLET_H

#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/http_controller.h"

namespace token{
#define FOR_EACH_WALLET_CONTROLLER_ENDPOINT(V) \
  V(GET, "/wallet/:user", GetUserWallet)       \
  V(GET, "/wallet/:user/tokens/:hash", GetUserWalletTokenCode) \
  V(POST, "/wallet/:user/spend", PostUserWalletSpend)

  class WalletController : HttpController{
   protected:
#define DECLARE_ENDPOINT(Method, Path, Name) \
    HTTP_CONTROLLER_ENDPOINT(Name);

    FOR_EACH_WALLET_CONTROLLER_ENDPOINT(DECLARE_ENDPOINT)
#undef DECLARE_ENDPOINT
   public:
    WalletController():
      HttpController(){}
    ~WalletController() = default;

    bool Initialize(HttpRouter* router){
#define REGISTER_ENDPOINT(Method, Path, Name) \
      HTTP_CONTROLLER_##Method(Path, Name);

      FOR_EACH_WALLET_CONTROLLER_ENDPOINT(REGISTER_ENDPOINT)
#undef REGISTER_ENDPOINT
      return true;
    }
  };
}

#endif//TOKEN_ENABLE_REST_SERVICE
#endif//TOKEN_HTTP_CONTROLLER_WALLET_H