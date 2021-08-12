#ifndef TOKEN_RPC_MESSAGE_VERACK_H
#define TOKEN_RPC_MESSAGE_VERACK_H

#include "message_verack.pb.h"
#include "rpc/rpc_message_handshake.h"

namespace token{
  namespace rpc{
    typedef internal::proto::rpc::VerackMessage RawVerackMessage;

    class VerackMessage : public HandshakeMessage<RawVerackMessage>{
    public:
      class Builder : public HandshakeMessageBuilderBase<VerackMessage>{
      public:
        explicit Builder(RawVerackMessage* raw):
          HandshakeMessageBuilderBase<VerackMessage>(raw){}
        Builder() = default;
        ~Builder() override = default;

        VerackMessagePtr Build() const override{
          return std::make_shared<VerackMessage>(*raw_);
        }
      };
    public:
      VerackMessage() = default;
      VerackMessage(const Timestamp& timestamp, const ClientType& type, const Version& version, const Hash& nonce, const UUID& node_id):
        HandshakeMessage<RawVerackMessage>(timestamp, type, version, nonce, node_id){}
      explicit VerackMessage(internal::proto::rpc::VerackMessage raw):
        HandshakeMessage<RawVerackMessage>(std::move(raw)){}
      explicit VerackMessage(const internal::BufferPtr& data, const uint64_t& msize):
        HandshakeMessage<RawVerackMessage>(data, msize){}
      ~VerackMessage() override = default;

      Type type() const override{
        return Type::kVerackMessage;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "VerackMessage(";
        ss << "timestamp=" << FormatTimestampReadable(timestamp()) << ", ";
        ss << "client_type=" << client_type() << ", ";
        ss << "version=" << version() << ", ";
        ss << "nonce=" << nonce() << ", ";
        ss << "node_id=" << node_id();
        ss << ")";
        return ss.str();
      }

      VerackMessage& operator=(const VerackMessage& rhs) = default;

      friend std::ostream& operator<<(std::ostream& stream, const VerackMessage& rhs){
        return stream << rhs.ToString();
      }

      static inline VerackMessagePtr
      NewInstance(const Timestamp&  timestamp, const ClientType& client_type, const Version& version, const Hash& nonce, const UUID& node_id){
        Builder builder;
        builder.SetTimestamp(timestamp);
        builder.SetClientType(client_type);
        builder.SetVersion(version);
        builder.SetNonce(nonce);
        builder.SetNodeId(node_id);
        return builder.Build();
      }

      static inline VerackMessagePtr
      Decode(const BufferPtr& data, const uint64_t& msize){
        return std::make_shared<VerackMessage>(data, msize);
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_VERACK_H