#ifndef TOKEN_RPC_MESSAGE_VERSION_H
#define TOKEN_RPC_MESSAGE_VERSION_H

#include "message_version.pb.h"
#include "rpc/rpc_message_handshake.h"

namespace token{
  namespace rpc{
  class VersionMessage : public HandshakeMessage<internal::proto::rpc::VersionMessage>{
    public:
      class Builder : public HandshakeMessageBuilderBase<VersionMessage>{
      public:
        explicit Builder(internal::proto::rpc::VersionMessage* raw):
          HandshakeMessageBuilderBase<VersionMessage>(raw){}
        Builder() = default;
        ~Builder() override = default;

        VersionMessagePtr Build() const override{
          return std::make_shared<VersionMessage>(*raw_);
        }
      };
     public:
      VersionMessage():
        HandshakeMessage(){}
      explicit VersionMessage(internal::proto::rpc::VersionMessage raw):
        HandshakeMessage(std::move(raw)){}
      ~VersionMessage() override = default;

      Type type() const override{
        return Type::kVersionMessage;
      }

      internal::BufferPtr ToBuffer() const override{
        NOT_IMPLEMENTED(ERROR);//TODO: implement
        return nullptr;
      }

      std::string ToString() const override{
        NOT_IMPLEMENTED(ERROR);//TODO: implement
        return "VersionMessage()";
      }

      VersionMessage& operator=(const VersionMessage& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const VersionMessage& msg){
        return stream << msg.ToString();
      }

      static inline VersionMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        return nullptr;
      }
    };
  }
}
#endif//TOKEN_RPC_MESSAGE_VERSION_H