#ifndef TOKEN_RPC_MESSAGE_REJECTED_H
#define TOKEN_RPC_MESSAGE_REJECTED_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class RejectedMessage : public PaxosMessage{
    public:
    class Encoder : public codec::PaxosMessageEncoder<RejectedMessage>{
      public:
        Encoder(const RejectedMessage* value, const codec::EncoderFlags& flags):
          codec::PaxosMessageEncoder<RejectedMessage>(value, flags){}
        Encoder(const Encoder& rhs) = default;
        ~Encoder() override = default;
        Encoder& operator=(const Encoder& rhs) = default;
      };

    class Decoder : public codec::PaxosMessageDecoder<RejectedMessage>{
      public:
        explicit Decoder(const codec::DecoderHints& hints):
          codec::PaxosMessageDecoder<RejectedMessage>(hints){}
        Decoder(const Decoder& rhs) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, RejectedMessage& result) const override;
        Decoder& operator=(const Decoder& rhs) = default;
      };
     public:
      RejectedMessage() = default;
      explicit RejectedMessage(const Proposal& proposal):
        PaxosMessage(proposal){}
      ~RejectedMessage() override = default;

      Type type() const override{
        return Type::kRejectedMessage;
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
        ss << "RejectedMessage(" << proposal() << ")";
        return ss.str();
      }

      RejectedMessage& operator=(const RejectedMessage& other) = default;

      static inline RejectedMessagePtr
      NewInstance(const Proposal& proposal){
        return std::make_shared<RejectedMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, RejectedMessage& result, const codec::DecoderHints& hints){
        Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline RejectedMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints){
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