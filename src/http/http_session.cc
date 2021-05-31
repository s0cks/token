#include "http/http_session.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace token{
  namespace http{
    void Session::OnMessageRead(const MessagePtr& msg){
      RequestPtr request = std::static_pointer_cast<Request>(msg);
      RouterMatch match = router_->Find(request);
      if(match.IsNotFound()){
        std::stringstream ss;
        ss << "Cannot find: " << request->GetPath();
        return Send(NewNotFoundResponse(shared_from_this(), ss));
      } else if(match.IsNotSupported()){
        std::stringstream ss;
        ss << "Method Not Supported for: " << request->GetPath();
        return Send(NewNotSupportedResponse(shared_from_this(), ss));
      } else{
        assert(match.IsOk());

        // weirdness :(
        request->SetPathParameters(match.GetPathParameters());
        request->SetQueryParameters(match.GetQueryParameters());

        Route& route = match.GetRoute();
        RouteHandlerFunction& handler = route.GetHandler();
        handler(route.GetController(), shared_from_this(), request);
      }
    }
  }
}