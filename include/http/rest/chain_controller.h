#ifndef TOKEN_REST_CHAIN_CONTROLLER_H
#define TOKEN_REST_CHAIN_CONTROLLER_H

#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/controller.h"

namespace Token{
  class BlockChainController : HttpController{
   private:
    BlockChainController() = delete;

    HTTP_CONTROLLER_ENDPOINT(GetBlockChain);
    HTTP_CONTROLLER_ENDPOINT(GetBlockChainHead);
    HTTP_CONTROLLER_ENDPOINT(GetBlockChainBlock);
   public:
    ~BlockChainController() = delete;

    HTTP_CONTROLLER_INIT(){
      HTTP_CONTROLLER_GET("/chain", GetBlockChain);
      HTTP_CONTROLLER_GET("/chain/head", GetBlockChainHead);
      HTTP_CONTROLLER_GET("/chain/data/:hash", GetBlockChainBlock);
    }
  };
}

#endif//TOKEN_ENABLE_REST_SERVICE
#endif//TOKEN_REST_CHAIN_CONTROLLER_H