#ifdef TOKEN_ENABLE_REST_SERVICE

#include "server/http/service.h"

#include "server/http/controller/chain_controller.h"
#include "server/http/controller/pool_controller.h"
#include "server/http/controller/wallet_controller.h"
#include "server/http/controller/health_controller.h"

namespace Token{
  HttpMessagePtr HttpMessage::From(HttpSession* session, const BufferPtr& buffer){
    HttpMessagePtr message = HttpRequest::NewInstance(session, buffer);
    buffer->SetReadPosition(buffer->GetBufferSize());
    return message;
  }

  HttpRestService::HttpRestService(uv_loop_t* loop):
    HttpService(loop){
    HealthController::Initialize(&router_);
    WalletController::Initialize(&router_);
    ObjectPoolController::Initialize(&router_);
    BlockChainController::Initialize(&router_);
  }

  static HttpRestService instance;

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

  void HttpSession::OnMessageRead(const HttpMessagePtr& msg){
    HttpRequestPtr request = std::static_pointer_cast<HttpRequest>(msg);
    HttpRouter* router = HttpRestService::GetServiceRouter();

    HttpRouterMatch match = router->Find(request);
    if(match.IsNotFound()){
      std::stringstream ss;
      ss << "Cannot find: " << request->GetPath();
      return Send(NewNotFoundResponse(this, ss));
    } else if(match.IsNotSupported()){
      std::stringstream ss;
      ss << "Method Not Supported for: " << request->GetPath();
      return Send(NewNotSupportedResponse(this, ss));
    } else{
      assert(match.IsOk());

      // weirdness :(
      request->SetPathParameters(match.GetPathParameters());
      request->SetQueryParameters(match.GetQueryParameters());
      HttpRouteHandler handler = match.GetHandler();
      handler(this, request);
    }
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE