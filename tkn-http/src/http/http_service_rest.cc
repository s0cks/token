#include "blockchain.h"
#include "../../include/http/http_service_rest.h"

namespace token{
  namespace http{
    RestService::RestService(uv_loop_t* loop,
                             const ObjectPoolPtr& pool,
                             const BlockChainPtr& chain,
                             const WalletManagerPtr& wallets):
     ServiceBase(loop),
     pool_(PoolController::NewInstance(pool)),
     chain_(ChainController::NewInstance(chain)),
     wallets_(WalletController::NewInstance(wallets)){
      if(!GetPoolController()->Initialize(GetRouter()))
        LOG(WARNING) << "couldn't initialize the pool controller.";

      if(!GetChainController()->Initialize(GetRouter()))
        LOG(WARNING) << "couldn't initialize the chain controller.";

      if(!GetWalletController()->Initialize(GetRouter()))
        LOG(WARNING) << "couldn't initialize the wallet controller.";
    }

    RestServicePtr RestService::NewInstance(){
      return std::make_shared<RestService>(uv_loop_new(), ObjectPool::GetInstance(), BlockChain::GetInstance(), WalletManager::GetInstance());
    }

    RestServicePtr RestService::NewInstance(uv_loop_t* loop,
                                            const ObjectPoolPtr& pool,
                                            const BlockChainPtr& chain,
                                            const WalletManagerPtr& wallets){
      return std::make_shared<RestService>(loop, pool, chain, wallets);
    }
  }
}