#ifndef TOKEN_RPC_SESSION_H
#define TOKEN_RPC_SESSION_H

#include "session.h"
#include "rpc/rpc_messages.h"
#include "rpc/rpc_message_handler.h"

namespace token{
  namespace rpc{
    class Message;

    class Session : public SessionBase{
      friend class PeerSession;
     protected:
      Session() = default;
      explicit Session(uv_loop_t* loop, const UUID& uuid=UUID()):
        SessionBase(loop, uuid){}

      virtual rpc::MessageHandler& GetMessageHandler() = 0;
     public:
      ~Session() override = default;

      virtual void Send(const rpc::MessageList& messages){
        return SendMessages((const std::vector<std::shared_ptr<MessageBase>>&)messages);
      }

      virtual void Send(const rpc::MessagePtr& msg){
        std::vector<std::shared_ptr<MessageBase>> messages = { msg };
        return SendMessages(messages);
      }

      virtual bool SendPrepare() = 0;
      virtual bool SendPromise() = 0;
      virtual bool SendCommit() = 0;
      virtual bool SendAccepted() = 0;
      virtual bool SendRejected() = 0;
    };
  }
}

#endif//TOKEN_RPC_SESSION_H