#include "blockchain.h"
#include "http/http_service_rest.h"

namespace token{
  HttpRestService* HttpRestService::GetInstance(){
    static HttpRestService instance;
    return &instance;
  }

  HttpRestService::HttpRestService(uv_loop_t* loop):
    HttpService(loop, GetThreadName()),
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

  static ThreadId thread_;

  bool HttpRestServiceThread::Start(){
    return ThreadStart(&thread_, HttpRestService::GetThreadName(), &HandleThread, (uword)HttpRestService::GetInstance());
  }

  bool HttpRestServiceThread::Join(){
    return ThreadJoin(thread_);
  }

  void HttpRestServiceThread::HandleThread(uword param){
    auto instance = (HttpRestService*)param;
    DLOG_THREAD_IF(ERROR, !instance->Run()) << "failed to run loop";
    pthread_exit(nullptr);
  }
}