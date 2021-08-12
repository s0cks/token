#ifndef TOKEN_RPC_MESSAGE_VERSION_H
#define TOKEN_RPC_MESSAGE_VERSION_H

#include <utility>

#include "message_version.pb.h"
#include "rpc/rpc_message_handshake.h"

namespace token{
  namespace rpc{
    typedef internal::proto::rpc::VersionMessage RawVersionMessage;

  class VersionMessage : public HandshakeMessage<RawVersionMessage>{
    public:
      class Builder : public HandshakeMessageBuilderBase<VersionMessage>{
      public:
        explicit Builder(RawVersionMessage* raw):
          HandshakeMessageBuilderBase<VersionMessage>(raw){}
        Builder() = default;
        ~Builder() override = default;

        VersionMessagePtr Build() const override{
          return std::make_shared<VersionMessage>(*raw_);
        }
      };
     public:
      VersionMessage() = default;
      VersionMessage(const Timestamp& timestamp, const ClientType& type, const Version& version, const Hash& nonce, const UUID& node_id):
        HandshakeMessage<RawVersionMessage>(timestamp, type, version, nonce, node_id){}
      explicit VersionMessage(RawVersionMessage raw):
        HandshakeMessage<RawVersionMessage>(std::move(raw)){}
      explicit VersionMessage(const internal::BufferPtr& data, const uint64_t& msize):
        HandshakeMessage<RawVersionMessage>(data, msize){}
      ~VersionMessage() override = default;

      Type type() const override{
        return Type::kVersionMessage;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "VersionMessage(";
        ss << "timestamp=" << FormatTimestampReadable(timestamp()) << ", ";
        ss << "client_type=" << client_type() << ", ";
        ss << "version=" << version() << ", ";
        ss << "nonce=" << nonce() << ", ";
        ss << "node_id=" << node_id();
        ss << ")";
        return ss.str();
      }

      VersionMessage& operator=(const VersionMessage& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const VersionMessage& msg){
        return stream << msg.ToString();
      }

      static inline VersionMessagePtr
      NewInstance(const Timestamp&  timestamp, const ClientType& client_type, const Version& version, const Hash& nonce, const UUID& node_id){
        Builder builder;
        builder.SetTimestamp(timestamp);
        builder.SetClientType(client_type);
        builder.SetVersion(version);
        builder.SetNonce(nonce);
        builder.SetNodeId(node_id);
        return builder.Build();
      }

      static inline VersionMessagePtr
      Decode(const BufferPtr& data, const uint64_t& msize){
        return std::make_shared<VersionMessage>(data, msize);
      }
    };
  }
}
#endif//TOKEN_RPC_MESSAGE_VERSION_H