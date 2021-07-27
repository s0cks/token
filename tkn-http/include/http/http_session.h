#ifndef TOKEN_HTTP_SESSION_H
#define TOKEN_HTTP_SESSION_H

#include "server/session.h"
#include "http/http_common.h"
#include "http/http_router.h"
#include "http/http_message.h"

namespace token{
  namespace http{
    class ServiceBase;
    class Session : public SessionBase{
     public:
      Session():
        SessionBase(){}
      Session(uv_loop_t* loop, const UUID& session_id):
        SessionBase(loop, session_id){}
      explicit Session(uv_loop_t* loop):
        Session(loop, UUID()){}
      ~Session() override = default;

      virtual void OnMessageRead(const internal::BufferPtr& buff) = 0;

      void Send(const http::HttpMessagePtr& msg){
        //TODO: log?
        return SendMessages({msg});
      }
    };
  }
}

#endif//TOKEN_HTTP_SESSION_H