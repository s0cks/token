#ifndef TOKEN_HTTP_SESSION_H
#define TOKEN_HTTP_SESSION_H

#include "http/router.h"
#include "http/message.h"
#include "server/session.h"

namespace Token{
  class HttpSession : public Session<HttpMessage>{
    friend class Server<HttpMessage, HttpSession>;
   private:
    HttpRouter* router_;

    void OnMessageRead(const HttpMessagePtr& msg);
   public:
    HttpSession(uv_loop_t* loop, HttpRouter* router):
      Session(loop),
      router_(router){
      handle_.data = this;
    }
    ~HttpSession() = default;

    void Send(const HttpMessagePtr& msg){
      return SendMessages({ msg });
    }
  };
}

#endif//TOKEN_HTTP_SESSION_H