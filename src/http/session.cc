#include "http/session.h"
#include "http/request.h"
#include "http/response.h"

namespace Token{
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
      HttpRouteHandler handler = match.GetHandler();
      handler(this, request);
    }
  }
}