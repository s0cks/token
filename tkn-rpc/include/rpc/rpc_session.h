#ifndef TOKEN_RPC_SESSION_H
#define TOKEN_RPC_SESSION_H

#include "session.h"
#include "rpc/rpc_message_handler.h"

namespace token{
  namespace rpc{
    class Session : public SessionBase{
     protected:
      Session() = default;
      explicit Session(uv_loop_t* loop, const UUID& uuid=UUID()):
        SessionBase(loop, uuid){}
     public:
      ~Session() override = default;

      virtual void Send(const rpc::MessageList& messages){
        return SendMessages((const std::vector<std::shared_ptr<MessageBase>>&)messages);
      }

      virtual void Send(const rpc::MessagePtr& msg){
        std::vector<std::shared_ptr<MessageBase>> messages = { msg };
        return SendMessages(messages);
      }
    };
  }
}

#endif//TOKEN_RPC_SESSION_H