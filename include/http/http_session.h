#ifndef TOKEN_HTTP_SESSION_H
#define TOKEN_HTTP_SESSION_H

#include "session.h"
#include "http/http_common.h"
#include "http/http_router.h"
#include "http/http_message.h"

namespace token{
  namespace http{
    class Session : public SessionBase{
     private:
      RouterPtr router_;

      void OnMessageRead(const BufferPtr& buff) override;
     public:
      Session():
        SessionBase(),
        router_(Router::NewInstance()){}
      explicit Session(uv_loop_t* loop):
        SessionBase(loop, UUID()),
        router_(Router::NewInstance()){}
      Session(uv_loop_t* loop, const RouterPtr& router):
        SessionBase(loop, UUID()),
        router_(router){}
      ~Session() override = default;

      virtual void Send(const http::HttpMessagePtr& msg){
        return SendMessages({msg});
      }
    };
  }
}

#endif//TOKEN_HTTP_SESSION_H