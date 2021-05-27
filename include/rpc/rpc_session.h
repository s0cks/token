#ifndef TOKEN_RPC_SESSION_H
#define TOKEN_RPC_SESSION_H

#include "session.h"
#include "rpc/rpc_messages.h"
#include "rpc/rpc_message_handler.h"

namespace token{
  namespace rpc{
    class Message;

    class Session : public SessionBase<rpc::Message>{
      friend class LedgerServer;
     protected:
      Session() = default;
      explicit Session(uv_loop_t* loop):
         SessionBase<rpc::Message>(loop, UUID()){}

      virtual rpc::MessageHandler& GetMessageHandler() = 0;

      void OnMessageRead(const rpc::MessagePtr& msg) override{
        switch(msg->GetType()) {
#define DEFINE_HANDLE(Name) \
        case Type::k##Name##Message: \
          return GetMessageHandler().On##Name##Message(std::static_pointer_cast<rpc::Name##Message>(msg));

          FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLE)
#undef DEFINE_HANDLE
          default: return;
        }
      }
     public:
      ~Session() override = default;

      virtual void Send(const SessionMessageTypeList& messages){
        return SendMessages(messages);
      }

      virtual void Send(const SessionMessageTypePtr& msg){
        SessionMessageTypeList messages = { msg };
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