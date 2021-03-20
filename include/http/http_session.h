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

    void OnMessageRead(const HttpMessagePtr& msg) override;
   public:
    HttpSession():
      Session<HttpMessage>(),
      router_(nullptr){}
    explicit HttpSession(uv_loop_t* loop):
      Session<HttpMessage>(loop, UUID()),
      router_(nullptr){}
    HttpSession(uv_loop_t* loop, HttpRouter* router):
      Session<HttpMessage>(loop, UUID()),
      router_(router){}
    ~HttpSession() override = default;

    virtual void Send(const HttpMessagePtr& msg){
      return SendMessages({ msg });
    }
  };
}

#endif//TOKEN_HTTP_SESSION_H