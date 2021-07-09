#ifndef TOKEN_RPC_MESSAGE_ACCEPTS_H
#define TOKEN_RPC_MESSAGE_ACCEPTS_H

#include "network/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class AcceptsMessage : public PaxosMessage{
     public:
      class Encoder : public PaxosMessageEncoder<PaxosMessage>{
       public:
        explicit Encoder(const AcceptsMessage& value, const codec::EncoderFlags& flags=GetDefaultMessageEncoderFlags()):
          PaxosMessageEncoder<PaxosMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;

        int64_t GetBufferSize() const override{
          return PaxosMessageEncoder::GetBufferSize();
        }

        bool Encode(const BufferPtr& buff) const override{
          return PaxosMessageEncoder::Encode(buff);
        }

        Encoder& operator=(const Encoder& other) = default;
      };

      class Decoder : public PaxosMessageDecoder<AcceptsMessage>{
       public:
        explicit Decoder(const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()):
          PaxosMessageDecoder<AcceptsMessage>(hints){}
          Decoder(const Decoder& other) = default;
        ~Decoder() override = default;

        bool Decode(const BufferPtr& buff, AcceptsMessage& result) const override{
          NOT_IMPLEMENTED(ERROR);
          return false;
        }

        Decoder& operator=(const Decoder& other) = default;
      };
     public:
      AcceptsMessage():
        PaxosMessage(){}
      explicit AcceptsMessage(const RawProposal& proposal):
        PaxosMessage(proposal){}
      ~AcceptsMessage() override = default;

      Type type() const override{
        return Type::kAcceptsMessage;
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
        ss << "AcceptsMessage(" << proposal() << ")";
        return ss.str();
      }

      AcceptsMessage& operator=(const AcceptsMessage& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const AcceptsMessage& msg){
        return stream << msg.ToString();
      }

      static inline AcceptsMessagePtr
      NewInstance(const RawProposal& proposal){
        return std::make_shared<AcceptsMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, AcceptsMessage& result, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline AcceptsMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        AcceptsMessage instance;
        if(!Decode(buff, instance, hints)){
          DLOG(ERROR) << "cannot decode AcceptsMessage.";
          return nullptr;
        }
        return std::make_shared<AcceptsMessage>(instance);
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_ACCEPTS_H