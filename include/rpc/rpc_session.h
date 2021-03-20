#ifndef TOKEN_RPC_SESSION_H
#define TOKEN_RPC_SESSION_H

#include "session.h"
#include "rpc/rpc_message.h"

namespace token{
  template<class M>
  class Server;

  class RpcSession : public Session<RpcMessage>{
    friend class LedgerServer;
   protected:
    RpcSession():
      Session<RpcMessage>(){}
    explicit RpcSession(uv_loop_t* loop):
      Session<RpcMessage>(loop, UUID()){}

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void On##Name##Message(const Name##MessagePtr& message) = 0;
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

    void OnMessageRead(const RpcMessagePtr& message) override{
      switch(message->GetType()){
#define DEFINE_HANDLE(Name) \
        case Type::k##Name##Message: \
          On##Name##Message(std::static_pointer_cast<Name##Message>(message)); \
          break;
        FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLE)
#undef DEFINE_HANDLE
        default:
          break;
      }
    }
   public:
    ~RpcSession() override = default;

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

#endif//TOKEN_RPC_SESSION_H