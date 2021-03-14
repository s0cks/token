#include "http/http_session.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace token{
  void HttpSession::OnMessageRead(const HttpMessagePtr& msg){
    HttpRequestPtr request = std::static_pointer_cast<HttpRequest>(msg);
    HttpRouterMatch match = router_->Find(request);
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

      HttpRoute& route = match.GetRoute();
      HttpRouteHandler handler = route.GetHandler();
      handler(route.GetOwner(), this, request);
    }
  }
}