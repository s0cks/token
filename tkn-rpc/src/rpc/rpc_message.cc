#include "rpc/rpc_session.h"
#include "rpc/rpc_messages.h"

namespace token{
  namespace rpc{
    void MessageHandler::Send(const rpc::MessagePtr& msg) const{
      return GetSession()->Send(msg);
    }

    void MessageHandler::Send(const rpc::MessageList& msgs) const{
      return GetSession()->Send(msgs);
    }

    MessagePtr MessageParser::Next() const{
      uint64_t mtype = data_->GetUnsignedLong();
      DLOG(INFO) << "decoded mtype: " << mtype;

      Type type = static_cast<Type>(mtype);
      DECODED_FIELD(type_, Type, type);

      uint64_t msize = data_->GetUnsignedLong();
      DECODED_FIELD(msize_, uint64_t, msize);

      switch(type){
#define DEFINE_DECODE(Name) \
        case Type::k##Name##Message: \
          return std::static_pointer_cast<Message>(Name##Message::Decode(data_, msize));
        FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
        default:
          DLOG(ERROR) << "cannot decode message of type: " << type;
          return nullptr;
      }
    }
  }
}