#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/rest_service.h"

#include "http/controller/pool_controller.h"
#include "http/controller/chain_controller.h"
#include "http/controller/wallet_controller.h"

namespace Token{
  static HttpRestService instance;

  HttpRestService::HttpRestService(uv_loop_t* loop):
    HttpService(loop){
    ObjectPoolController::Initialize(&router_);
    BlockChainController::Initialize(&router_);
    WalletController::Initialize(&router_);
  }

  bool HttpRestService::Start(){
    return instance.StartThread();
  }

  bool HttpRestService::Shutdown(){
    LOG(WARNING) << "HttpRestService::Shutdown() not implemented.";
    return false; //TODO: implement HttpRestService::Shutdown()
  }

  bool HttpRestService::WaitForShutdown(){
    return instance.JoinThread();
  }

  HttpRouter* HttpRestService::GetServiceRouter(){
    return instance.GetRouter();
  }

#define DEFINE_STATE_CHECK(Name) \
  bool HttpRestService::IsService##Name(){ return instance.Is##Name(); }
  FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
}

#endif//TOKEN_ENABLE_REST_SERVICE