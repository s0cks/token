#ifndef TOKEN_RPC_MESSAGE_HANDLER_H
#define TOKEN_RPC_MESSAGE_HANDLER_H

#include "rpc/rpc_message.h"

namespace token{
  namespace rpc{
    class Session;

    class MessageHandler{
    protected:
      rpc::Session* session_;

      explicit MessageHandler(rpc::Session* session):
         session_(session){}
    public:
      virtual ~MessageHandler() = default;

      rpc::Session* GetSession() const{
        return session_;
      }

      void Send(const rpc::MessagePtr& msg) const;
      void Send(const rpc::MessageList& msgs) const;

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void On##Name##Message(const Name##MessagePtr& message) = 0; //TODO: change to const?

      FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
    };
  }
}

#endif //TOKEN_RPC_MESSAGE_HANDLER_H
