#ifndef TOKEN_HTTP_SESSION_H
#define TOKEN_HTTP_SESSION_H

#include "http/http_message.h"
#include "session.h"

namespace token{
  class HttpRouter;
  class HttpSession : public Session<HttpMessage>{
    friend class HttpService;
   private:
    HttpRouter* router_;

    void OnMessageRead(const HttpMessagePtr& msg);
   public:
    HttpSession(uv_loop_t* loop, HttpRouter* router):
      Session(loop),
      router_(router){}
    ~HttpSession() = default;

    void Send(const HttpMessagePtr& msg){
      return SendMessages({ msg });
    }
  };
}

#endif//TOKEN_HTTP_SESSION_H