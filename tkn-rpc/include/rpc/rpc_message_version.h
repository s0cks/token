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

      class Decoder : public codec::HandshakeMessageDecoder<rpc::VersionMessage>{
      public:
        explicit Decoder(const codec::DecoderHints& hints):
          codec::HandshakeMessageDecoder<rpc::VersionMessage>(hints){}
        Decoder(const Decoder& rhs) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, rpc::VersionMessage& result) const;
        Decoder& operator=(const Decoder& rhs) = default;
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

      static inline bool
      Decode(const BufferPtr& buff, VersionMessage& result, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline VersionMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        VersionMessage msg;
        if(!Decode(buff, msg, hints)){
          DLOG(ERROR) << "cannot decode VersionMessage.";
          return nullptr;
        }
        return std::make_shared<VersionMessage>(msg);
      }
    };
  }
}
#endif//TOKEN_RPC_MESSAGE_VERSION_H