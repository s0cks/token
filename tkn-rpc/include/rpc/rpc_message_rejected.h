#ifndef TOKEN_RPC_MESSAGE_REJECTED_H
#define TOKEN_RPC_MESSAGE_REJECTED_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class RejectedMessage;
  }

  namespace codec{
    class RejectedMessageEncoder : public PaxosMessageEncoder<rpc::RejectedMessage>{
    public:
      RejectedMessageEncoder(const rpc::RejectedMessage& value, const codec::EncoderFlags& flags):
        PaxosMessageEncoder<rpc::RejectedMessage>(value, flags){}
      RejectedMessageEncoder(const RejectedMessageEncoder& rhs) = default;
      ~RejectedMessageEncoder() override = default;
      RejectedMessageEncoder& operator=(const RejectedMessageEncoder& rhs) = default;
    };

    class RejectedMessageDecoder : public PaxosMessageDecoder<rpc::RejectedMessage>{
    public:
      RejectedMessageDecoder(const codec::DecoderHints& hints):
        PaxosMessageDecoder<rpc::RejectedMessage>(hints){}
      RejectedMessageDecoder(const RejectedMessageDecoder& rhs) = default;
      ~RejectedMessageDecoder() override = default;
      bool Decode(const BufferPtr& buff, rpc::RejectedMessage& result) const override;
      RejectedMessageDecoder& operator=(const RejectedMessageDecoder& rhs) = default;
    };
  }

  namespace rpc{
    class RejectedMessage : public PaxosMessage{
     public:
      RejectedMessage() = default;
      explicit RejectedMessage(const Proposal& proposal):
        PaxosMessage(proposal){}
      ~RejectedMessage() override = default;

      Type type() const override{
        return Type::kRejectedMessage;
      }

      int64_t GetBufferSize() const override{
        codec::RejectedMessageEncoder encoder((*this), GetDefaultMessageEncoderFlags());
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        codec::RejectedMessageEncoder encoder((*this), GetDefaultMessageEncoderFlags());
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
      Decode(const BufferPtr& buff, RejectedMessage& result, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        codec::RejectedMessageDecoder decoder(hints);
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