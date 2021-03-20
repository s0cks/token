#ifndef TOKEN_CONTROLLER_H
#define TOKEN_CONTROLLER_H

#include "http_router.h"
#include "http_request.h"
#include "http_response.h"

namespace token{
  class HttpController{
   protected:
    HttpController() = default;
   public:
    virtual ~HttpController() = default;
    virtual bool Initialize(HttpRouter* router) = 0;
  };

#define HTTP_CONTROLLER_ENDPOINT(Endpoint) \
  static void Handle##Endpoint(HttpController*, HttpSession*, const HttpRequestPtr&); \
  virtual void On##Endpoint(HttpSession* session, const HttpRequestPtr& request);

#define HTTP_CONTROLLER_ROUTE_HANDLER(Controller, Name) \
  void Controller::Handle##Name(HttpController* instance, HttpSession* session, const HttpRequestPtr& request){ \
    return ((Controller*)instance)->On##Name(session, request);                                                 \
  }

#define HTTP_CONTROLLER_ENDPOINT_HANDLER(Controller, Name) \
  void Controller::On##Name(HttpSession* session, const HttpRequestPtr& request)

#define HTTP_CONTROLLER_GET(Path, Name) \
    router->Get(this, Path, &Handle##Name)

#define HTTP_CONTROLLER_PUT(Path, Name) \
    router->Put(this, Path, &Handle##Name)

#define HTTP_CONTROLLER_POST(Path, Name) \
    router->Post(this, Path, &Handle##Name)

#define HTTP_CONTROLLER_DELETE(Path, Name) \
    router->Delete(this, Path, &Handle##Name)
}

#endif //TOKEN_CONTROLLER_H