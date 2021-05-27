#include "pool.h"
#include "rpc/rpc_session.h"
#include "rpc/rpc_message.h"
#include "rpc/rpc_messages.h"

namespace token{
  namespace rpc{
    void MessageHandler::Send(const rpc::MessagePtr& msg) const{
      return GetSession()->Send(msg);
    }

    void MessageHandler::Send(const rpc::MessageList& msgs) const{
      return GetSession()->Send(msgs);
    }
  }

  bool rpc::Message::Write(const BufferPtr& buff) const{
    return buff->PutObjectTag(GetTag())
        && WriteMessage(buff);
  }

  rpc::MessagePtr rpc::Message::From(SessionBase<rpc::Message>* session, const BufferPtr& buffer){
    ObjectTag tag = buffer->GetObjectTag();
    switch(tag.GetType()){
#define DEFINE_DECODE(Name) \
      case Type::k##Name##Message:  \
        return rpc::Name##Message::NewInstance(buffer);
      FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
      default:
        LOG(ERROR) << "unknown message: " << tag;
        return nullptr;
    }
  }
}