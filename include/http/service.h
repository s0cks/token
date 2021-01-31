#ifndef TOKEN_HTTP_SERVICE_H
#define TOKEN_HTTP_SERVICE_H

#include "server.h"
#include "http/router.h"
#include "http/message.h"
#include "http/session.h"

namespace Token{
  class HttpService : public Server<HttpMessage, HttpSession>{
   protected:
    HttpRouter router_;

    HttpService(uv_loop_t* loop):
      Server(loop, "http/service"),
      router_(){}

    HttpSession* CreateSession() const{
      return new HttpSession(GetLoop(), (HttpRouter*)&router_);
    }
   public:
    virtual ~HttpService() = default;

    ServerPort GetPort() const{
      return FLAGS_service_port;
    }

    HttpRouter* GetRouter(){
      return &router_;
    }
  };
}

#endif//TOKEN_HTTP_SERVICE_H