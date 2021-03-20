#include "blockchain.h"
#include "http/http_service_rest.h"

namespace token{
  static HttpRestService instance;

  HttpRestService::HttpRestService(uv_loop_t* loop):
    HttpService(loop),
    pool_(std::make_shared<PoolController>(ObjectPool::GetInstance())),
    chain_(std::make_shared<ChainController>(BlockChain::GetInstance())),
    wallet_(std::make_shared<WalletController>(WalletManager::GetInstance())){

    if(!GetPoolController()->Initialize(&router_))
      LOG(WARNING) << "couldn't initialize the pool controller.";

    if(!GetChainController()->Initialize(&router_))
      LOG(WARNING) << "couldn't initialize the chain controller.";

    if(!GetWalletController()->Initialize(&router_))
      LOG(WARNING) << "couldn't initialize the wallet controller.";
  }

  bool HttpRestService::Start(){
    return instance.StartThread();
  }

  bool HttpRestService::Shutdown(){
    return instance.SendShutdown();
  }

  bool HttpRestService::WaitForShutdown(){
    return instance.JoinThread();
  }

#define DEFINE_STATE_CHECK(Name) \
  bool HttpRestService::IsService##Name(){ return instance.Is##Name(); }
  FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
}