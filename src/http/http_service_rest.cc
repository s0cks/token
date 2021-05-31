#include "blockchain.h"
#include "http/http_service_rest.h"

namespace token{
  namespace http{
    RestService::RestService(uv_loop_t* loop):
      ServiceBase(loop, GetThreadName()),
      pool_(std::make_shared<PoolController>(ObjectPool::GetInstance())),
      chain_(std::make_shared<ChainController>(BlockChain::GetInstance())),
      wallets_(std::make_shared<WalletController>(WalletManager::GetInstance())){

      if(!GetPoolController()->Initialize(GetRouter()))
        LOG(WARNING) << "couldn't initialize the pool controller.";

      if(!GetChainController()->Initialize(GetRouter()))
        LOG(WARNING) << "couldn't initialize the chain controller.";

      if(!GetWalletController()->Initialize(GetRouter()))
        LOG(WARNING) << "couldn't initialize the wallet controller.";
    }
  }
}