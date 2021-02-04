#ifndef TOKEN_HTTP_SERVICE_H
#define TOKEN_HTTP_SERVICE_H

#include "server.h"
#include "http/http_router.h"
#include "http/http_message.h"
#include "http/http_session.h"

namespace token{
  class HttpService : public Server<HttpMessage>{
   protected:
    HttpRouter router_;

    HttpService(uv_loop_t* loop):
      Server(loop, "http/service"),
      router_(){}

    Session<HttpMessage>* CreateSession() const{
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