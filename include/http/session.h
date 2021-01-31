#ifndef TOKEN_HTTP_SESSION_H
#define TOKEN_HTTP_SESSION_H

#include "session.h"
#include "http/message.h"

namespace Token{
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