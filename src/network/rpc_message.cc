#include "pool.h"
#include "network/rpc_session.h"
#include "network/rpc_message.h"
#include "network/rpc_messages.h"

namespace token{
  namespace rpc{
    void MessageHandler::Send(const rpc::MessagePtr& msg) const{
      return GetSession()->Send(msg);
    }

    void MessageHandler::Send(const rpc::MessageList& msgs) const{
      return GetSession()->Send(msgs);
    }

    MessagePtr Message::From(const BufferPtr& buffer){
/*    ObjectTag tag = buffer->GetObjectTag();
    switch(tag.GetType()){
#define DEFINE_DECODE(Name) \
      case Type::k##Name##Message:  \
        return rpc::Name##Message::NewInstance(buffer);
      FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
      default:
        LOG(ERROR) << "unknown message: " << tag;
        return nullptr;*/
      NOT_IMPLEMENTED(FATAL);
      return nullptr;
    }
  }
}