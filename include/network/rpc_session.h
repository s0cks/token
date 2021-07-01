#ifndef TOKEN_RPC_SESSION_H
#define TOKEN_RPC_SESSION_H

#include "session.h"
#include "network/rpc_common.h"
#include "network/rpc_messages.h"
#include "network/rpc_message_handler.h"

namespace token{
  namespace rpc{
    class Session : public SessionBase{
      friend class PeerSession;
     protected:
#define DECLARE_ASYNC_HANDLE(Name) \
      uv_async_t send_##Name##_;
      FOR_EACH_MESSAGE_TYPE(DECLARE_ASYNC_HANDLE)
#undef DECLARE_ASYNC_HANDLE

      Session() = default;
      explicit Session(uv_loop_t* loop, const UUID& uuid=UUID()):
        SessionBase(loop, uuid){}
     public:
      ~Session() override = default;

      virtual rpc::MessageHandler& GetMessageHandler() = 0;

      virtual void Send(const rpc::MessageList& messages){
        return SendMessages((const std::vector<std::shared_ptr<MessageBase>>&)messages);
      }

      virtual void Send(const rpc::MessagePtr& msg){
        std::vector<std::shared_ptr<MessageBase>> messages = { msg };
        return SendMessages(messages);
      }

#define DEFINE_ASYNC_SEND(Name) \
      inline void Send##Name(){ uv_async_send(&send_##Name##_); }
      FOR_EACH_MESSAGE_TYPE(DEFINE_ASYNC_SEND)
#undef DEFINE_ASYNC_SEND
    };
  }
}

#endif//TOKEN_RPC_SESSION_H