#ifndef TOKEN_CONTROLLER_H
#define TOKEN_CONTROLLER_H

#include "http_request.h"
#include "http_response.h"

namespace token{
  namespace http{
    class Controller{
     protected:
      Controller() = default;
     public:
      virtual ~Controller() = default;
      virtual bool Initialize(const RouterPtr& router) = 0;
    };
  }

#define HTTP_CONTROLLER_ENDPOINT(Endpoint) \
  static void Handle##Endpoint(const ControllerPtr&, http::Session*, const RequestPtr&); \
  virtual void On##Endpoint(http::Session*, const RequestPtr& request);

#define HTTP_CONTROLLER_ROUTE_HANDLER(Controller, Name) \
  void Controller::Handle##Name(const ControllerPtr& instance, http::Session* session, const RequestPtr& request){ \
    return std::static_pointer_cast<Controller>(instance)->On##Name(session, request);                                \
  }

#define HTTP_CONTROLLER_ENDPOINT_HANDLER(Controller, Name) \
  void Controller::On##Name(http::Session* session, const RequestPtr& request)

#define HTTP_CONTROLLER_GET(Path, Name) \
    router->Get(shared_from_this(), Path, &Handle##Name)

#define HTTP_CONTROLLER_PUT(Path, Name) \
    router->Put(shared_from_this(), Path, &Handle##Name)

#define HTTP_CONTROLLER_POST(Path, Name) \
    router->Post(shared_from_this(), Path, &Handle##Name)

#define HTTP_CONTROLLER_DELETE(Path, Name) \
    router->Delete(shared_from_this(), Path, &Handle##Name)
}

#endif //TOKEN_CONTROLLER_H