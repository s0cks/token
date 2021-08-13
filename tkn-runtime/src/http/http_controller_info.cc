#include "runtime.h"
#include "http/http_session.h"
#include "http/http_controller_info.h"

namespace token{
  namespace http{
#define DEFINE_ENDPOINT_ROUTE_HANDLER(Method, Path, Name) \
    HTTP_CONTROLLER_ROUTE_HANDLER(InfoController, Name)
    FOR_EACH_INFO_CONTROLLER_ENDPOINT(DEFINE_ENDPOINT_ROUTE_HANDLER)
#undef DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER

    HTTP_CONTROLLER_ENDPOINT_HANDLER(InfoController, GetRuntimeInfo){
      RuntimeInfo info(runtime_);

      json::String body;
      json::Writer writer(body);
      if(!json::SetField(writer, "data", info))
        return session->Send(NewInternalServerErrorResponse("cannot get runtime info."));
      return session->Send(NewOkResponse(body));
    }
  }
}