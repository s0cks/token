#ifndef TOKEN_RPC_SESSION_H
#define TOKEN_RPC_SESSION_H

#include "session.h"

namespace Token{
  class RpcSession : public Session<RpcMessage>{
   protected:
    RpcSession(uv_loop_t* loop):
      Session<RpcMessage>(loop){}

#define DECLARE_MESSAGE_HANDLER(Name) \
    void On##Name##Message(const Name##MessagePtr& message) = 0;
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

    void OnMessageRead(const RpcMessagePtr& message){
      switch(message->GetMessageType()){
#define DEFINE_HANDLE(Name) \
        case RpcMessage::MessageType::k##Name##MessageType: \
          On##Name##Message(std::static_pointer_cast<Name##Message>(message)); \
          break;
        FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLE)
#undef DEFINE_HANDLE
        default:
          break;
      }
    }
   public:
    virtual ~RpcSession() = default;

    inline void Send(const RpcMessageList& messages){
      return SendMessages(messages);
    }

    inline void Send(const RpcMessagePtr& msg){
      RpcMessageList messages = { msg };
      return SendMessages(messages);
    }
  };
}

#endif//TOKEN_RPC_SESSION_H