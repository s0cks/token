#ifndef TOKEN_RPC_MESSAGE_VERSION_H
#define TOKEN_RPC_MESSAGE_VERSION_H

#include <ostream>
#include "network/rpc_message_handshake.h"

namespace token{
  namespace rpc{
    class VersionMessage : public HandshakeMessage{
     public:
      class Encoder : public HandshakeMessage::HandshakeMessageEncoder<VersionMessage>{
       public:
        Encoder(const VersionMessage& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
          HandshakeMessageEncoder<VersionMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;

        int64_t GetBufferSize() const override{
          return HandshakeMessageEncoder::GetBufferSize();
        }

        bool Encode(const BufferPtr& buffer) const override{
          return HandshakeMessageEncoder::Encode(buffer);
        }

        Encoder& operator=(const Encoder& other) = default;
      };

      class Decoder : public HandshakeMessageDecoder<VersionMessage>{
       public:
        Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
          HandshakeMessageDecoder<VersionMessage>(hints){}
        Decoder(const Decoder& other) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buffer, VersionMessage& result) const override;
        Decoder& operator=(const Decoder& other) = default;
      };
     private:
      BlockHeader head_;
     public:
      VersionMessage():
        HandshakeMessage(),
        head_(){}
      VersionMessage(const Timestamp& timestamp,
                     const ClientType& client_type,
                     const Version& version,
                     const Hash& nonce,
                     const UUID& node_id,
                     const BlockHeader& head):
        HandshakeMessage(timestamp, client_type, version, nonce, node_id),
        head_(head){}
      VersionMessage(const VersionMessage& other) = default;
      ~VersionMessage() = default;

      Type type() const override{
        return Type::kVersionMessage;
      }

      BlockHeader& head(){
        return head_;
      }

      BlockHeader head() const{
        return head_;
      }

      int64_t GetBufferSize() const override{
        Encoder encoder((*this));
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        Encoder encoder((*this));
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
        ss << "head=" << head_;
        ss << ")";
        return ss.str();
      }

      VersionMessage& operator=(const VersionMessage& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const VersionMessage& msg){
        return stream << msg.ToString();
      }

      static inline VersionMessagePtr
      NewInstance(const Timestamp& timestamp, const ClientType& client_type, const Version& version, const Hash& nonce, const UUID& node_id, const BlockHeader& head){
        return std::make_shared<VersionMessage>(timestamp, client_type, version, nonce, node_id, head);
      }

      static inline bool
      Decode(const BufferPtr& buff, VersionMessage& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline VersionMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
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