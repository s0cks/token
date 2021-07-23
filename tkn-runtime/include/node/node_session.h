#ifndef TKN_NODE_SESSION_H
#define TKN_NODE_SESSION_H

#include "rpc/rpc_session.h"
#include "node/node_session_message_handler.h"

namespace token{
  namespace node{
    class Session : public rpc::Session{
    private:
      SessionMessageHandler handler_;

      SessionMessageHandler& handler(){
        return handler_;
      }
    public:
      explicit Session(uv_loop_t* loop):
        rpc::Session(loop),
        handler_(this){}
      ~Session() override = default;

      void OnMessageRead(const internal::BufferPtr& data){
        HandleMessages(data, handler());
      }
    };
  }
}

#endif//TKN_NODE_SESSION_H