#ifndef TKN_PEER_SESSION_MESSAGE_HANDLER_H
#define TKN_PEER_SESSION_MESSAGE_HANDLER_H

#include "rpc/rpc_message_handler.h"

namespace token{
  namespace peer{
    class Session;

    class SessionMessageHandler : public rpc::MessageHandler{
    public:
      explicit SessionMessageHandler(peer::Session* session):
        rpc::MessageHandler((rpc::Session*)session){}
      ~SessionMessageHandler() override = default;

#define DECLARE_HANDLE(Name) \
      void On##Name##Message(const rpc::Name##MessagePtr& msg) override;
      FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLE)
#undef DECLARE_HANDLE//DECLARE_HANDLE
    };
  }
}

#endif//TKN_PEER_SESSION_MESSAGE_HANDLER_H