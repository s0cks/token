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
      Type type = static_cast<Type>(data_->GetUnsignedLong());
      DLOG(INFO) << "decoded message type: " << type;

      Version version(data_->GetShort(), data_->GetShort(), data_->GetShort());
      DLOG(INFO) << "decoded message version: " << version;

      codec::DecoderHints hints = codec::ExpectTypeHint::Encode(true)|codec::ExpectVersionHint::Encode(true);
      switch(type){
#define DEFINE_DECODE(Name) \
        case Type::k##Name##Message: \
          return std::static_pointer_cast<Message>(Name##Message::DecodeNew(data_, hints));
        FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
        default:
          DLOG(ERROR) << "cannot decode message of type: " << type;
          return nullptr;
      }
    }

    MessagePtr Message::From(const BufferPtr& buffer){

    }
  }
}