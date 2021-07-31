#ifndef TOKEN_RPC_MESSAGE_VERSION_H
#define TOKEN_RPC_MESSAGE_VERSION_H

#include <utility>

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
      VersionMessage() = default;
      explicit VersionMessage(internal::proto::rpc::VersionMessage raw):
        HandshakeMessage<internal::proto::rpc::VersionMessage>(std::move(raw)){}
      ~VersionMessage() override = default;

      Type type() const override{
        return Type::kVersionMessage;
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
      NewInstance(const Timestamp&  timestamp, const ClientType& client_type, const Version& version, const Hash& nonce, const UUID& node_id){
        internal::proto::rpc::VersionMessage raw;

        Builder builder(&raw);
        builder.SetTimestamp(timestamp);
        builder.SetClientType(client_type);
        //TODO: builder.SetVersion(version);
        //TODO: builder.SetNonce(nonce);
        //TODO: builder.SetNodeId(node_id);

        return builder.Build();
      }

      static inline VersionMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        return nullptr;
      }
    };
  }
}
#endif//TOKEN_RPC_MESSAGE_VERSION_H