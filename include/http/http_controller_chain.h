#ifndef TOKEN_HTTP_CONTROLLER_CHAIN_H
#define TOKEN_HTTP_CONTROLLER_CHAIN_H

#include "blockchain.h"
#include "http/http_controller.h"

namespace token{
  namespace http{
    class ChainController;
    typedef std::shared_ptr<ChainController> ChainControllerPtr;

#define FOR_EACH_CHAIN_CONTROLLER_ENDPOINT(V) \
  V(GET, "/chain", GetBlockChain)             \
  V(GET, "/chain/head", GetBlockChainHead)    \
  V(GET, "/chain/data/:hash", GetBlockChainBlock)

    class ChainController : public Controller,
                            public std::enable_shared_from_this<ChainController>{
     protected:
      BlockChainPtr chain_;
     public:
      explicit ChainController(const BlockChainPtr& chain):
          Controller(),
          chain_(chain){}
      ~ChainController() override = default;

      BlockChainPtr GetChain() const{
        return chain_;
      }

#define DECLARE_ENDPOINT(Method, Path, Name) \
    HTTP_CONTROLLER_ENDPOINT(Name);

      FOR_EACH_CHAIN_CONTROLLER_ENDPOINT(DECLARE_ENDPOINT)
#undef DECLARE_ENDPOINT

      bool Initialize(const RouterPtr& router) override{
#define REGISTER_ENDPOINT(Method, Path, Name) \
      HTTP_CONTROLLER_##Method(Path, Name);

        FOR_EACH_CHAIN_CONTROLLER_ENDPOINT(REGISTER_ENDPOINT)
#undef REGISTER_ENDPOINT
        return true;
      }

      static inline ChainControllerPtr
      NewInstance(const BlockChainPtr& chain=BlockChain::GetInstance()){
        return std::make_shared<ChainController>(chain);
      }
    };

    static inline ChainControllerPtr
    NewChainController(const BlockChainPtr& chain=BlockChain::GetInstance()){
      return std::make_shared<ChainController>(chain);
    }
  }
}

#endif//TOKEN_HTTP_CONTROLLER_CHAIN_H