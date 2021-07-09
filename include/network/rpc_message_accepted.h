#ifndef TOKEN_RPC_MESSAGE_ACCEPTED_H
#define TOKEN_RPC_MESSAGE_ACCEPTED_H

#include "network/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class AcceptedMessage : public PaxosMessage{
     public:
      class Encoder : public PaxosMessageEncoder<AcceptedMessage>{
       public:
        explicit Encoder(const AcceptedMessage& value, const codec::EncoderFlags& flags=GetDefaultMessageEncoderFlags()):
          PaxosMessageEncoder<AcceptedMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;

        int64_t GetBufferSize() const override{
          return PaxosMessageEncoder<AcceptedMessage>::GetBufferSize();
        }

        bool Encode(const BufferPtr& buff) const override{
          return PaxosMessageEncoder<AcceptedMessage>::Encode(buff);
        }

        Encoder& operator=(const Encoder& other) = default;
      };

      class Decoder : public PaxosMessageDecoder<AcceptedMessage>{
       public:
        explicit Decoder(const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()):
          PaxosMessageDecoder<AcceptedMessage>(hints){}
        Decoder(const Decoder& other) = default;
        ~Decoder() override = default;

        bool Decode(const BufferPtr& buff, AcceptedMessage& result) const{
          NOT_IMPLEMENTED(ERROR);
          return false;
        }

        Decoder& operator=(const Decoder& other) = default;
      };
     public:
      AcceptedMessage():
        PaxosMessage(){}
      explicit AcceptedMessage(const RawProposal& proposal):
        PaxosMessage(proposal){}
      ~AcceptedMessage() override = default;

      Type type() const override{
        return Type::kAcceptedMessage;
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
        ss << "AcceptedMessage(" << proposal() << ")";
        return ss.str();
      }

      AcceptedMessage& operator=(const AcceptedMessage& other) = default;

      static inline AcceptedMessagePtr
      NewInstance(const RawProposal& proposal){
        return std::make_shared<AcceptedMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, AcceptedMessage& result, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline AcceptedMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        AcceptedMessage instance;
        if(!Decode(buff, instance, hints)){
          DLOG(ERROR) << "cannot decode AcceptedMessage.";
          return nullptr;
        }
        return std::make_shared<AcceptedMessage>(instance);
      }
    };

    static inline std::ostream&
    operator<<(std::ostream& stream, const AcceptedMessagePtr& msg){
      return stream << msg->ToString();
    }

    static inline MessageList&
    operator<<(MessageList& list, const AcceptedMessagePtr& msg){
      list.push_back(msg);
      return list;
    }
  }
}

#endif//TOKEN_RPC_MESSAGE_ACCEPTED_H