#include "http/http_session.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace token{
  namespace http{
    void Session::OnMessageRead(const BufferPtr& buff){
      auto handler = new AsyncRequestHandler(GetLoop(), this, router_, buff);
      handler->Execute();
    }

    RequestPtr AsyncRequestHandler::ParseRequest(){
      DLOG(INFO) << "parsing http request....";
      auto request = RequestParser::ParseRequest(buffer_);
      DLOG(INFO) << "parsed http request: " << request->ToString();
      return request;
    }

    RouterMatch AsyncRequestHandler::FindMatch(const RequestPtr& request){
      return router_->Find(request);
    }

    void AsyncRequestHandler::DoWork(uv_async_t* work){
      auto handler = (AsyncRequestHandler*)work->data;
      auto session = (http::Session*)handler->session_;
      auto request = handler->ParseRequest();
      auto match = handler->FindMatch(request);

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

        Route &route = match.GetRoute();
        route.GetHandler()(route.GetController(), session, request);
      }
    }
  }
}