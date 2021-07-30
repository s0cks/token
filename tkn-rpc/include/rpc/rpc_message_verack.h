#ifndef TOKEN_RPC_MESSAGE_VERACK_H
#define TOKEN_RPC_MESSAGE_VERACK_H

#include "message_verack.pb.h"
#include "rpc/rpc_message_handshake.h"

namespace token{
  namespace rpc{
    class VerackMessage : public HandshakeMessage<internal::proto::rpc::VerackMessage>{
    public:
      class Builder : public HandshakeMessageBuilderBase<VerackMessage>{
      public:
        explicit Builder(internal::proto::rpc::VerackMessage* raw):
          HandshakeMessageBuilderBase<VerackMessage>(raw){}
        Builder() = default;
        ~Builder() override = default;

        VerackMessagePtr Build() const override{
          return std::make_shared<VerackMessage>(*raw_);
        }
      };
    public:
      VerackMessage() = default;
      explicit VerackMessage(internal::proto::rpc::VerackMessage raw):
        HandshakeMessage<internal::proto::rpc::VerackMessage>(std::move(raw)){}
      ~VerackMessage() override = default;

      Type type() const override{
        return Type::kVerackMessage;
      }

      int64_t GetBufferSize() const override{
        return 0;
      }

      bool Write(const BufferPtr& data) const override{
        return false;
      }

      std::string ToString() const override{
        NOT_IMPLEMENTED(ERROR);//TODO: implement
        return "VerackMessage()";
      }

      VerackMessage& operator=(const VerackMessage& rhs) = default;

      friend std::ostream& operator<<(std::ostream& stream, const VerackMessage& rhs){
        return stream << rhs.ToString();
      }

      static inline VerackMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        NOT_IMPLEMENTED(ERROR);//TODO: implement
        return nullptr;
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_VERACK_H