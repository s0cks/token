#include "pool.h"
#include "rpc/rpc_message.h"

namespace token{
  bool RpcMessage::Write(const BufferPtr& buff) const{
    return buff->PutObjectTag(GetTag())
        && WriteMessage(buff);
  }

  RpcMessagePtr RpcMessage::From(Session<RpcMessage>* session, const BufferPtr& buffer){
    ObjectTag tag = buffer->GetObjectTag();
    switch(tag.GetType()){
#define DEFINE_DECODE(Name) \
      case Type::k##Name##Message:  \
        return Name##Message::NewInstance(buffer);
      FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
      default:
        LOG(ERROR) << "unknown message: " << tag;
        return nullptr;
    }
  }
}