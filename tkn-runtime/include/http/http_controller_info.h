#ifndef TKN_HTTP_CONTROLLER_INFO_H
#define TKN_HTTP_CONTROLLER_INFO_H

#include "http/http_controller.h"

namespace token{
  class Runtime;
  namespace http{
#define FOR_EACH_INFO_CONTROLLER_ENDPOINT(V) \
    V(GET, "/info", GetRuntimeInfo)

    class InfoController : public Controller{
      Runtime* runtime_;
    public:
      explicit InfoController(Runtime* runtime):
        Controller(),
        runtime_(runtime){}
      ~InfoController() override = default;
      DECLARE_HTTP_CONTROLLER(InfoController, FOR_EACH_INFO_CONTROLLER_ENDPOINT);
    };
  }
}

#endif//TKN_HTTP_CONTROLLER_INFO_H