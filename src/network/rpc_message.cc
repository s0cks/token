#include "pool.h"
#include "network/rpc_session.h"
#include "network/rpc_message.h"
#include "network/rpc_messages.h"

namespace token{
  namespace rpc{
    BlockChainPtr MessageHandler::GetChain() const{
      return GetSession()->GetChain();
    }

    void MessageHandler::Send(const rpc::MessagePtr& msg) const{
      return GetSession()->Send(msg);
    }

    void MessageHandler::Send(const rpc::MessageList& msgs) const{
      return GetSession()->Send(msgs);
    }

    class MessageParser{
     private:
      BufferPtr data_;
     public:
      MessageParser(BufferPtr& data):
        data_(data){}
      ~MessageParser() = default;

      bool HasNext() const{
        return false;
      }

      MessagePtr Next() const{
        return nullptr;
      }
    };

    MessagePtr Message::From(const BufferPtr& buffer){
      Type type = static_cast<Type>(buffer->GetUnsignedLong());
      DLOG(INFO) << "decoded message type: " << type;

      Version version = buffer->GetVersion();
      DLOG(INFO) << "decoded message version: " << version;

      codec::DecoderHints hints = codec::ExpectTypeHint::Encode(true)|codec::ExpectVersionHint::Encode(true);
      switch(type){
#define DEFINE_DECODE(Name) \
        case Type::k##Name##Message: \
          return Name##Message::DecodeNew(buffer, hints);
        FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
        default:
          DLOG(ERROR) << "cannot decode message of type: " << type;
          return nullptr;
      }
    }
  }
}