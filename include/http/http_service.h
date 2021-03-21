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

    HttpService(uv_loop_t* loop, const char* name):
      Server(loop, name),
      router_(){}

    Session<HttpMessage>* CreateSession() const override{
      return new HttpSession(GetLoop(), (HttpRouter*)&router_);
    }
   public:
    ~HttpService() override = default;

    ServerPort GetPort() const override{
      return FLAGS_service_port;
    }

    HttpRouter* GetRouter(){
      return &router_;
    }
  };
}

#endif//TOKEN_HTTP_SERVICE_H