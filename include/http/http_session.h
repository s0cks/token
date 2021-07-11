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
      BufferPtr rbuffer_;

      RouterPtr router_;
     public:
      Session():
        SessionBase(),
        rbuffer_(),
        router_(Router::NewInstance()){}
      explicit Session(uv_loop_t* loop):
        SessionBase(loop, UUID()),
        rbuffer_(),
        router_(Router::NewInstance()){}
      Session(uv_loop_t* loop, const RouterPtr& router):
        SessionBase(loop, UUID()),
        rbuffer_(internal::NewBuffer(65536)),
        router_(router){}
      ~Session() override = default;

      void OnMessageRead(const BufferPtr& buff);

      void Send(const http::HttpMessagePtr& msg){
        DLOG(INFO) << "sending: " << msg->ToString();
        return SendMessages({msg});
      }
    };

    class AsyncRequestHandler{
     private:
      uv_async_t request_;
      Session* session_;
      RouterPtr router_;
      BufferPtr buffer_;

      RequestPtr ParseRequest();
      RouterMatch FindMatch(const RequestPtr& request);

      static void DoWork(uv_async_t* request);
     public:
      explicit AsyncRequestHandler(uv_loop_t* loop, Session* session, const RouterPtr& router, const BufferPtr& buff):
        request_(),
        session_(session),
        router_(router),
        buffer_(buff){
        request_.data = this;
        CHECK_UVRESULT2(ERROR, uv_async_init(loop, &request_, &DoWork), "couldn't initialize async handle.");
      }
      ~AsyncRequestHandler() = default;

      bool Execute(){
        VERIFY_UVRESULT2(ERROR, uv_async_send(&request_), "couldn't send async request.");
        return true;
      }
    };
  }
}

#endif//TOKEN_HTTP_SESSION_H