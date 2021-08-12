#ifndef TKN_NODE_SESSION_H
#define TKN_NODE_SESSION_H

#include "rpc/rpc_session.h"
#include "node/node_session_message_handler.h"

namespace token{
  namespace node{
    class Server;
    class Session : public rpc::Session{
    private:
      Server* server_;
      SessionMessageHandler handler_;
      uv_async_t on_send_promise_;
      uv_async_t on_send_accepted_;
      uv_async_t on_send_rejected_;

      SessionMessageHandler& handler(){
        return handler_;
      }

      static void OnSendPromise(uv_async_t* handle);
      static void OnSendAccepted(uv_async_t* handle);
      static void OnSendRejected(uv_async_t* handle);
    public:
      explicit Session(Server* server);
      ~Session() override = default;

      Server* GetServer() const{
        return server_;
      }

      void OnMessageRead(internal::BufferPtr& data){
        HandleMessages(data, handler());
      }

      inline bool
      SendPromise(){
        VERIFY_UVRESULT2(FATAL, uv_async_send(&on_send_promise_), "cannot invoke on_send_promise_ callback");
        return true;
      }

      inline bool
      SendAccepted(){
        VERIFY_UVRESULT2(FATAL, uv_async_send(&on_send_accepted_), "cannot invoke on_send_accepted_ callback");
        return true;
      }

      inline bool
      SendRejected(){
        VERIFY_UVRESULT2(FATAL, uv_async_send(&on_send_rejected_), "cannot invoke on_send_rejected_ callback");
        return true;
      }
    };
  }
}

#endif//TKN_NODE_SESSION_H