#include "http/http_service_rest.h"

namespace token{
  namespace http{
    RestService::RestService(uv_loop_t* loop,
                             const ObjectPoolPtr& pool):
     ServiceBase(loop),
     pool_(PoolController::NewInstance(pool)){

      if(!GetPoolController()->Initialize(GetRouter()))
        LOG(WARNING) << "couldn't initialize the pool controller.";

//      if(!GetChainController()->Initialize(GetRouter()))
//        LOG(WARNING) << "couldn't initialize the chain controller.";
//
//      if(!GetWalletController()->Initialize(GetRouter()))
//        LOG(WARNING) << "couldn't initialize the wallet controller.";
    }

    RestServicePtr RestService::NewInstance(){
      return std::make_shared<RestService>(uv_loop_new(), ObjectPool::GetInstance());
    }

    RestServicePtr RestService::NewInstance(uv_loop_t* loop,
                                            const ObjectPoolPtr& pool){
      return std::make_shared<RestService>(loop, pool);
    }
  }
}