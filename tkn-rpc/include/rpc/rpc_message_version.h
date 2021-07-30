#ifndef TOKEN_RPC_MESSAGE_VERSION_H
#define TOKEN_RPC_MESSAGE_VERSION_H

#include "rpc/rpc_message_handshake.h"

namespace token{
  namespace rpc{
    class VersionMessage : public HandshakeMessage{
    public:
    class Encoder : public codec::HandshakeMessageEncoder<rpc::VersionMessage>{
      public:
        Encoder(const VersionMessage* value, const codec::EncoderFlags& flags):
          codec::HandshakeMessageEncoder<rpc::VersionMessage>(value, flags){}
        Encoder(const Encoder& rhs) = default;
        ~Encoder() override = default;
        Encoder& operator=(const Encoder& rhs) = default;
      };

      class Decoder : public codec::HandshakeMessageDecoder<VersionMessage>{
      public:
        explicit Decoder(const codec::DecoderHints& hints):
          codec::HandshakeMessageDecoder<VersionMessage>(hints){}
        ~Decoder() override = default;
        VersionMessage* Decode(const BufferPtr& data) const override;
      };
     public:
      VersionMessage():
        HandshakeMessage(){}
      VersionMessage(const Timestamp& timestamp,
                     const ClientType& client_type,
                     const Version& version,
                     const Hash& nonce,
                     const UUID& node_id):
        HandshakeMessage(timestamp, client_type, version, nonce, node_id){}
      VersionMessage(const VersionMessage& other) = default;
      ~VersionMessage() override = default;

      Type type() const override{
        return Type::kVersionMessage;
      }

      int64_t GetBufferSize() const override{
        Encoder encoder(this, GetDefaultMessageEncoderFlags());
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        Encoder encoder(this, GetDefaultMessageEncoderFlags());
        return encoder.Encode(buff);
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "VersionMessage(";
        ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
        ss << "client_type=" << client_type_ << ", ";
        ss << "version=" << version_ << ", ";
        ss << "nonce=" << nonce_ << ", ";
        ss << "node_id=" << node_id_ << ", ";
        ss << ")";
        return ss.str();
      }

      VersionMessage& operator=(const VersionMessage& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const VersionMessage& msg){
        return stream << msg.ToString();
      }

      static inline VersionMessagePtr
      NewInstance(const Timestamp& timestamp, const ClientType& client_type, const Version& version, const Hash& nonce, const UUID& node_id){
        return std::make_shared<VersionMessage>(timestamp, client_type, version, nonce, node_id);
      }

      static inline VersionMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);

        VersionMessage* value = nullptr;
        if(!(value = decoder.Decode(data))){
          DLOG(FATAL) << "cannot decode VersionMessage from buffer.";
          return nullptr;
        }
        return std::shared_ptr<VersionMessage>(value);
      }
    };
  }
}
#endif//TOKEN_RPC_MESSAGE_VERSION_H