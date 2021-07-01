#ifndef TOKEN_RPC_MESSAGE_PREPARE_H
#define TOKEN_RPC_MESSAGE_PREPARE_H

#include "network/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class PrepareMessage : public PaxosMessage{
     public:
      class Encoder : public PaxosMessageEncoder<PrepareMessage>{
       public:
        Encoder(const PrepareMessage& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
          PaxosMessageEncoder<PrepareMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;
        bool Encode(const BufferPtr& buff) const;
        Encoder& operator=(const Encoder& other) = default;
      };

      class Decoder : public PaxosMessageDecoder<PrepareMessage>{
       public:
        Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
          PaxosMessageDecoder<PrepareMessage>(hints){}
        Decoder(const Decoder& other) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, PrepareMessage& result) const override;
        Decoder& operator=(const Decoder& other) = default;
      };
     public:
      PrepareMessage():
        PaxosMessage(){}
      explicit PrepareMessage(const RawProposal& proposal):
        PaxosMessage(proposal){}
      PrepareMessage(const PrepareMessage& other) = default;
      ~PrepareMessage() override = default;

      Type type() const override{
        return Type::kPrepareMessage;
      }

      int64_t GetBufferSize() const override{
        Encoder encoder((*this));
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buffer) const override{
        Encoder encoder((*this));
        return encoder.Encode(buffer);
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "PrepareMessage(proposal=" << proposal() << ")";
        return ss.str();
      }

      PrepareMessage& operator=(const PrepareMessage& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const PrepareMessage& msg){
        return stream << msg.ToString();
      }

      static inline PrepareMessagePtr
      NewInstance(const RawProposal& proposal){
        return std::make_shared<PrepareMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, PrepareMessage& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline PrepareMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        PrepareMessage msg;
        if(!Decode(buff, msg, hints)){
          DLOG(ERROR) << "cannot decode PrepareMessage.";
          return nullptr;
        }
        return std::make_shared<PrepareMessage>(msg);
      }
    };

    static inline std::ostream&
    operator<<(std::ostream& stream, const PrepareMessagePtr& msg){
      return stream << msg->ToString();
    }

    static inline MessageList&
    operator<<(MessageList& list, const PrepareMessagePtr& msg){
      list.push_back(msg);
      return list;
    }
  }
}

#endif//TOKEN_RPC_MESSAGE_PREPARE_H