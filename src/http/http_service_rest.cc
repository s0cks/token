#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/http_service_rest.h"
#include "http/http_session.h"

#include "http/http_controller_pool.h"
#include "http/http_controller_chain.h"
#include "http/http_controller_wallet.h"

namespace Token{
  static HttpRestService instance;

  HttpRestService::HttpRestService(uv_loop_t* loop):
    HttpService(loop){
    PoolController::Initialize(&router_);
    ChainController::Initialize(&router_);
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