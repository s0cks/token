#ifndef TOKEN_RPC_MESSAGE_REJECTED_H
#define TOKEN_RPC_MESSAGE_REJECTED_H

#include "network/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class RejectedMessage : public PaxosMessage{
     public:
      class Encoder : public PaxosMessageEncoder<RejectedMessage>{
       public:
        explicit Encoder(const RejectedMessage& value, const codec::EncoderFlags& flags=GetDefaultMessageEncoderFlags()):
          PaxosMessageEncoder<RejectedMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;

        int64_t GetBufferSize() const override{
          return PaxosMessageEncoder<RejectedMessage>::GetBufferSize();
        }

        bool Encode(const BufferPtr& buff) const override{
          return PaxosMessageEncoder<RejectedMessage>::Encode(buff);
        }

        Encoder& operator=(const Encoder& other) = default;
      };

      class Decoder : public PaxosMessageDecoder<RejectedMessage>{
       public:
        explicit Decoder(const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()):
          PaxosMessageDecoder<RejectedMessage>(hints){}
        Decoder(const Decoder& other) = default;
        ~Decoder() override = default;

        bool Decode(const BufferPtr& buff, RejectedMessage& result) const{
          NOT_IMPLEMENTED(ERROR);
          return false;
        }

        Decoder& operator=(const Decoder& other) = default;
      };
     public:
      RejectedMessage() = default;
      explicit RejectedMessage(const RawProposal& proposal):
        PaxosMessage(proposal){}
      ~RejectedMessage() override = default;

      Type type() const override{
        return Type::kRejectedMessage;
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
        ss << "RejectedMessage(" << proposal() << ")";
        return ss.str();
      }

      RejectedMessage& operator=(const RejectedMessage& other) = default;

      static inline RejectedMessagePtr
      NewInstance(const RawProposal& proposal){
        return std::make_shared<RejectedMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, RejectedMessage& result, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline RejectedMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        RejectedMessage instance;
        if(!Decode(buff, instance, hints)){
          DLOG(ERROR) << "cannot decode RejectedMessage.";
          return nullptr;
        }
        return std::make_shared<RejectedMessage>(instance);
      }
    };

    static inline std::ostream&
    operator<<(std::ostream& stream, const RejectedMessagePtr& msg){
      return stream << msg->ToString();
    }

    static inline MessageList&
    operator<<(MessageList& list, const RejectedMessagePtr& msg){
      list.push_back(msg);
      return list;
    }
  }
}

#endif//TOKEN_RPC_MESSAGE_REJECTED_H