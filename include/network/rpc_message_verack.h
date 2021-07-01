#ifndef TOKEN_RPC_MESSAGE_VERACK_H
#define TOKEN_RPC_MESSAGE_VERACK_H

#include <ostream>
#include "network/rpc_message_handshake.h"

namespace token{
  namespace rpc{
    class VerackMessage : public rpc::HandshakeMessage{
     public:
      class Encoder : public HandshakeMessageEncoder<VerackMessage>{
       public:
        Encoder(const VerackMessage& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
          HandshakeMessageEncoder<VerackMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;

        int64_t GetBufferSize() const override{
          return HandshakeMessageEncoder<VerackMessage>::GetBufferSize();
        }

        bool Encode(const BufferPtr& buff) const override{
          return HandshakeMessageEncoder<VerackMessage>::Encode(buff);
        }

        Encoder& operator=(const Encoder& other) = default;
      };

      class Decoder : public HandshakeMessageDecoder<VerackMessage>{
       public:
        Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
          HandshakeMessageDecoder<VerackMessage>(hints){}
        Decoder(const Decoder& other) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, VerackMessage& result) const override;
        Decoder& operator=(const Decoder& other) = default;
      };
     private:
      BlockHeader head_;
      NodeAddress callback_;
     public:
      VerackMessage():
        HandshakeMessage(){}
      VerackMessage(const Timestamp& timestamp, const ClientType& client_type, const Version& version, const Hash& nonce, const UUID& node_id, const BlockHeader& head, const NodeAddress& callback):
        HandshakeMessage(timestamp, client_type, version, nonce, node_id),
        head_(head),
        callback_(callback){}
      VerackMessage(const VerackMessage& other) = default;
      ~VerackMessage() = default;

      Type type() const override{
        return Type::kVerackMessage;
      }

      NodeAddress& callback(){
        return callback_;
      }

      NodeAddress callback() const{
        return callback_;
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
        ss << "VerackMessage(";
        ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
        ss << "client_type=" << client_type_ << ", ";
        ss << "version=" << version_ << ", ";
        ss << "nonce=" << nonce_ << ", ";
        ss << "node_id=" << node_id_ << ", ";
        ss << "callback=" << callback_ << ", ";
        ss << "head=" << head_;
        ss << ")";
        return ss.str();
      }

      VerackMessage& operator=(const VerackMessage& other) = default;

      static inline VerackMessagePtr
      NewInstance(const Timestamp& timestamp, const ClientType& client_type, const Version& version, const Hash& nonce, const UUID& node_id, const BlockHeader& head, const NodeAddress& callback){
        return std::make_shared<VerackMessage>(timestamp, client_type, version, nonce, node_id, head, callback);
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