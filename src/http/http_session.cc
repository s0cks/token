#include "http/http_session.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace token{
  namespace http{
    void Session::OnMessageRead(const BufferPtr& buff){
      DLOG(INFO) << "parsing http request.";
      RequestPtr request = RequestParser::ParseRequest(buff);
      DLOG(INFO) << "received request " << request->ToString() << " from " << GetUUID();
      RouterMatch match = router_->Find(request);
      if(match.IsNotFound()){
        std::stringstream ss;
        ss << "Cannot find: " << request->GetPath();
        return Send(NewNotFoundResponse(ss));
      } else if(match.IsNotSupported()){
        std::stringstream ss;
        ss << "Method Not Supported for: " << request->GetPath();
        return Send(NewNotSupportedResponse(ss));
      } else{
        assert(match.IsOk());

        // weirdness :(
        request->SetPathParameters(match.GetPathParameters());
        request->SetQueryParameters(match.GetQueryParameters());

        Route& route = match.GetRoute();
        RouteHandlerFunction& handler = route.GetHandler();
        handler(route.GetController(), this, request);
      }
    }
  }
}