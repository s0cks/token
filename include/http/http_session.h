#ifndef TOKEN_HTTP_SESSION_H
#define TOKEN_HTTP_SESSION_H

#include "session.h"
#include "http/http_common.h"
#include "http/http_router.h"
#include "http/http_message.h"

namespace token{
  namespace http{
    class Router;
    class Session : public SessionBase<Message>,
                    public std::enable_shared_from_this<Session>{
     private:
      RouterPtr router_;

      void OnMessageRead(const MessagePtr& msg) override;
     public:
      Session():
        SessionBase<Message>(),
        router_(Router::NewInstance()){}
      explicit Session(uv_loop_t* loop):
        SessionBase<Message>(loop, UUID()),
        router_(Router::NewInstance()){}
      Session(uv_loop_t* loop, const RouterPtr& router):
        SessionBase<Message>(loop, UUID()),
        router_(router){}
      ~Session() override = default;

      virtual void Send(const http::HttpMessagePtr& msg){
        return SendMessages({msg});
      }
    };
  }
}

#endif//TOKEN_HTTP_SESSION_H