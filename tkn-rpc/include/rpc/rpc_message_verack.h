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

    class Decoder : public codec::HandshakeMessageDecoder<rpc::VerackMessage>{
      public:
        Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
            HandshakeMessageDecoder<rpc::VerackMessage>(hints){}
        Decoder(const Decoder& other) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, rpc::VerackMessage& result) const override;
        Decoder& operator=(const Decoder& other) = default;
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

      static inline bool
      Decode(const BufferPtr& buffer, VerackMessage& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);
        return decoder.Decode(buffer, result);
      }

      static inline VerackMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultEncoderFlags){
        VerackMessage instance;
        if(!Decode(buff, instance)){
          DLOG(ERROR) << "cannot decode VerackMessage.";
          return nullptr;
        }
        return std::make_shared<VerackMessage>(instance);
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_VERACK_H