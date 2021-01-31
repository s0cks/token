#ifndef TOKEN_HTTP_CONTROLLER_CHAIN_H
#define TOKEN_HTTP_CONTROLLER_CHAIN_H

#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/http_controller.h"

namespace Token{
  class ChainController : HttpController{
   private:
    ChainController() = delete;

    HTTP_CONTROLLER_ENDPOINT(GetBlockChain);
    HTTP_CONTROLLER_ENDPOINT(GetBlockChainHead);
    HTTP_CONTROLLER_ENDPOINT(GetBlockChainBlock);
   public:
    ~ChainController() = delete;

    HTTP_CONTROLLER_INIT(){
      HTTP_CONTROLLER_GET("/chain", GetBlockChain);
      HTTP_CONTROLLER_GET("/chain/head", GetBlockChainHead);
      HTTP_CONTROLLER_GET("/chain/data/:hash", GetBlockChainBlock);
    }
  };
}

#endif//TOKEN_ENABLE_REST_SERVICE
#endif//TOKEN_HTTP_CONTROLLER_CHAIN_H