#ifndef TOKEN_HTTP_CONTROLLER_CHAIN_H
#define TOKEN_HTTP_CONTROLLER_CHAIN_H

#include "blockchain.h"
#include "http/http_controller.h"

namespace token{
#define FOR_EACH_CHAIN_CONTROLLER_ENDPOINT(V) \
  V(GET, "/chain", GetBlockChain)             \
  V(GET, "/chain/head", GetBlockChainHead)    \
  V(GET, "/chain/data/:hash", GetBlockChainBlock)

  class ChainController : HttpController{
   protected:
    BlockChainPtr chain_;

    BlockChainPtr GetChain() const{
      return chain_;
    }
   public:
    ChainController(const BlockChainPtr& chain):
      HttpController(),
      chain_(chain){}
    ~ChainController() = default;

#define DECLARE_ENDPOINT(Method, Path, Name) \
    HTTP_CONTROLLER_ENDPOINT(Name);

    FOR_EACH_CHAIN_CONTROLLER_ENDPOINT(DECLARE_ENDPOINT)
#undef DECLARE_ENDPOINT

    bool Initialize(HttpRouter* router){
#define REGISTER_ENDPOINT(Method, Path, Name) \
      HTTP_CONTROLLER_##Method(Path, Name);

      FOR_EACH_CHAIN_CONTROLLER_ENDPOINT(REGISTER_ENDPOINT)
#undef REGISTER_ENDPOINT
      return true;
    }
  };
}

#endif//TOKEN_HTTP_CONTROLLER_CHAIN_H