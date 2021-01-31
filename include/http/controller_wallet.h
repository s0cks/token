#ifndef TOKEN_HTTP_CONTROLLER_WALLET_H
#define TOKEN_HTTP_CONTROLLER_WALLET_H

#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/controller.h"

namespace Token{
  class WalletController : HttpController{
   private:
    WalletController() = delete;

    HTTP_CONTROLLER_ENDPOINT(GetUserWallet);
    HTTP_CONTROLLER_ENDPOINT(PostUserWalletSpend);
    HTTP_CONTROLLER_ENDPOINT(GetUserWalletTokenCode);
   public:
    ~WalletController() = delete;

    HTTP_CONTROLLER_INIT(){
      HTTP_CONTROLLER_GET("/wallet/:user", GetUserWallet);
      HTTP_CONTROLLER_POST("/wallet/:user/spend", PostUserWalletSpend);
      HTTP_CONTROLLER_GET("/wallet/:user/tokens/:hash", GetUserWalletTokenCode);
    }
  };
}

#endif//TOKEN_ENABLE_REST_SERVICE
#endif//TOKEN_HTTP_CONTROLLER_WALLET_H