#ifndef TOKEN_RPC_MESSAGE_PROMISE_H
#define TOKEN_RPC_MESSAGE_PROMISE_H

#include "network/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class PromiseMessage : public PaxosMessage{
     public:
      class Encoder : public PaxosMessageEncoder<PromiseMessage>{
       public:
        explicit Encoder(const PromiseMessage& value, const codec::EncoderFlags& flags=GetDefaultMessageEncoderFlags()):
          PaxosMessageEncoder<PromiseMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;
        Encoder& operator=(const Encoder& other) = default;
      };

      class Decoder : public PaxosMessageDecoder<PromiseMessage>{
       public:
        explicit Decoder(const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()):
          PaxosMessageDecoder<PromiseMessage>(hints){}
        Decoder(const Decoder& other) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, PromiseMessage& result) const override;
        Decoder& operator=(const Decoder& other) = default;
      };
     public:
      PromiseMessage():
        PaxosMessage(){}
      explicit PromiseMessage(const RawProposal& proposal):
        PaxosMessage(proposal){}
      PromiseMessage(const PromiseMessage& other) = default;
      ~PromiseMessage() override = default;

      int64_t GetBufferSize() const override{
        Encoder encoder((*this));
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        Encoder encoder((*this));
        return encoder.Encode(buff);
      }

      Type type() const override{
        return Type::kPromiseMessage;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "PromiseMessage(" << proposal() << ")";
        return ss.str();
      }

      PromiseMessage& operator=(const PromiseMessage& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const PromiseMessage& msg){
        return stream << msg.ToString();
      }

      static inline PromiseMessagePtr
      NewInstance(const RawProposal& proposal){
        return std::make_shared<PromiseMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, PromiseMessage& result, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline PromiseMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        PromiseMessage msg;
        if(!Decode(buff, msg, hints)){
          DLOG(ERROR) << "cannot decode PromiseMessage.";
          return nullptr;
        }
        return std::make_shared<PromiseMessage>(msg);
      }
    };

    static inline std::ostream&
    operator<<(std::ostream& stream, const PromiseMessagePtr& msg){
      return stream << msg->ToString();
    }

    static inline MessageList&
    operator<<(MessageList& list, const PromiseMessagePtr& msg){
      list.push_back(msg);
      return list;
    }
  }
}

#endif//TOKEN_RPC_MESSAGE_PROMISE_H