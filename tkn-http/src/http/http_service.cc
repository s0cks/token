#include "http/http_service.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace token{
  namespace http{
    ServiceSession::ServiceSession(ServiceBase* service):
      ServiceSession(service->GetLoop(), service){}

    void ServiceSession::OnMessageRead(const internal::BufferPtr& buff){
      auto handler = new AsyncRequestHandler(GetLoop(), this, buff);//TODO: investigate allocation for this.
      if(!handler->Execute())
        DLOG(ERROR) << "cannot handle buffer.";//TODO: better error handling
    }

    RequestPtr AsyncRequestHandler::ParseRequest(){
      DLOG(INFO) << "parsing http request....";
      auto request = RequestParser::ParseRequest(buffer_);
      DLOG(INFO) << "parsed http request: " << request->ToString();
      return request;
    }

    RouterMatch AsyncRequestHandler::FindMatch(const RequestPtr& request){
      return session()->GetService()->GetRouter().Find(request);
    }

    void AsyncRequestHandler::DoWork(uv_async_t* work){
      auto handler = (AsyncRequestHandler*)work->data;
      auto session = handler->session();
      auto request = handler->ParseRequest();
      auto match = handler->FindMatch(request);

      DLOG(INFO) << "found match for path " << request->GetPath() << ": " << match.GetPath();
      if(match.IsNotFound()){
        std::stringstream ss;
        ss << "Cannot find: " << request->GetPath();
        return session->Send(NewNotFoundResponse(ss));
      } else if(match.IsNotSupported()){
        std::stringstream ss;
        ss << "Method Not Supported for: " << request->GetPath();
        return session->Send(NewNotSupportedResponse(ss));
      } else {
        assert(match.IsOk());

        // weirdness :(
        request->SetPathParameters(match.GetPathParameters());
        request->SetQueryParameters(match.GetQueryParameters());

        auto route = match.GetRoute();
        auto& endpoint = route->GetHandler();
        DLOG(INFO) << "invoking endpoint: " << route->GetPath();
        endpoint(route->GetController(), session, request);
      }
    }
  }
}