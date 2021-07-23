#ifndef TKN_NODE_SESSION_MESSAGE_HANDLER_H
#define TKN_NODE_SESSION_MESSAGE_HANDLER_H

#include "rpc/rpc_messages.h"
#include "rpc/rpc_message_handler.h"

namespace token{
  namespace node{
    class SessionMessageHandler: public rpc::MessageHandler{
    public:
      explicit SessionMessageHandler(rpc::Session* session):
        rpc::MessageHandler(session){}
      ~SessionMessageHandler() override = default;

#define DECLARE_HANDLE(Name) \
      void On##Name##Message(const rpc::Name##MessagePtr& msg) override;
      FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLE)
#undef DECLARE_HANDLE
    };
  }
}
#endif//TKN_NODE_SESSION_MESSAGE_HANDLER_H