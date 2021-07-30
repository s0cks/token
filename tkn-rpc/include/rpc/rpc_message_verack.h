#ifndef TOKEN_RPC_MESSAGE_VERACK_H
#define TOKEN_RPC_MESSAGE_VERACK_H

#include <ostream>
#include "rpc/rpc_message_handshake.h"

namespace token{
  namespace rpc{
    class VerackMessage : public rpc::HandshakeMessage{
    public:
    class Encoder : public codec::HandshakeMessageEncoder<rpc::VerackMessage>{
      public:
        Encoder(const VerackMessage* value, const codec::EncoderFlags& flags):
            HandshakeMessageEncoder<rpc::VerackMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;
        Encoder& operator=(const Encoder& other) = default;
      };

    class Decoder : public codec::HandshakeMessageDecoder<VerackMessage>{
      public:
        explicit Decoder(const codec::DecoderHints& hints):
            HandshakeMessageDecoder<VerackMessage>(hints){}
        ~Decoder() override = default;
        VerackMessage* Decode(const BufferPtr& data) const override;
      };
     private:
      //TODO: add head
      //TODO: add callback address
     public:
      VerackMessage():
        HandshakeMessage(){}
      VerackMessage(const Timestamp& timestamp, const ClientType& client_type, const Version& version, const Hash& nonce, const UUID& node_id):
        HandshakeMessage(timestamp, client_type, version, nonce, node_id){}
      VerackMessage(const VerackMessage& other) = default;
      ~VerackMessage() override = default;

      Type type() const override{
        return Type::kVerackMessage;
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
        ss << "VerackMessage(";
        ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
        ss << "client_type=" << client_type_ << ", ";
        ss << "version=" << version_ << ", ";
        ss << "nonce=" << nonce_ << ", ";
        ss << "node_id=" << node_id_ << ", ";
        ss << ")";
        return ss.str();
      }

      VerackMessage& operator=(const VerackMessage& other) = default;

      static inline VerackMessagePtr
      NewInstance(const Timestamp& timestamp, const ClientType& client_type, const Version& version, const Hash& nonce, const UUID& node_id){
        return std::make_shared<VerackMessage>(timestamp, client_type, version, nonce, node_id);
      }

      static inline VerackMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);

        VerackMessage* value = nullptr;
        if(!(value = decoder.Decode(data))){
          DLOG(FATAL) << "cannot decode VerackMessage from buffer.";
          return nullptr;
        }
        return std::shared_ptr<VerackMessage>(value);
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_VERACK_H