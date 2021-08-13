#ifndef TOKEN_CONTROLLER_H
#define TOKEN_CONTROLLER_H

#include "http_request.h"
#include "http_response.h"
#include "http/http_router.h"

namespace token{
  namespace http{
    class Controller{
     protected:
      Controller() = default;
     public:
      virtual ~Controller() = default;
      virtual bool Initialize(Router& router) = 0;
    };
  }

#define HTTP_CONTROLLER_ENDPOINT(Endpoint) \
  static void Handle##Endpoint(Controller*, http::Session*, const RequestPtr&); \
  virtual void On##Endpoint(http::Session*, const RequestPtr& request);

#define HTTP_CONTROLLER_ROUTE_HANDLER(Owner, Name) \
  void Owner::Handle##Name(Controller* instance, http::Session* session, const RequestPtr& request){ \
    return dynamic_cast<Owner*>(instance)->On##Name(session, request);                                      \
  }

#define HTTP_CONTROLLER_ENDPOINT_HANDLER(Owner, Name) \
  void Owner::On##Name(http::Session* session, const RequestPtr& request)

#define HTTP_CONTROLLER_GET(Path, Name) \
    router.Get(this, Path, &Handle##Name)

#define HTTP_CONTROLLER_PUT(Path, Name) \
    router.Put(this, Path, &Handle##Name)

#define HTTP_CONTROLLER_POST(Path, Name) \
    router.Post(this, Path, &Handle##Name)

#define HTTP_CONTROLLER_DELETE(Path, Name) \
    router.Delete(this, Path, &Handle##Name)
}

#endif //TOKEN_CONTROLLER_H