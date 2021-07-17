#ifndef TOKEN_RPC_MESSAGE_ACCEPTED_H
#define TOKEN_RPC_MESSAGE_ACCEPTED_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class AcceptedMessage;
  }

  namespace codec{
    class AcceptedMessageEncoder : public codec::PaxosMessageEncoder<rpc::AcceptedMessage>{
    public:
      explicit AcceptedMessageEncoder(const rpc::AcceptedMessage& value, const codec::EncoderFlags& flags):
        PaxosMessageEncoder<rpc::AcceptedMessage>(value, flags){}
      AcceptedMessageEncoder(const AcceptedMessageEncoder& other) = default;
      ~AcceptedMessageEncoder() override = default;
      AcceptedMessageEncoder& operator=(const AcceptedMessageEncoder& other) = default;
    };

    class AcceptedMessageDecoder : public PaxosMessageDecoder<rpc::AcceptedMessage>{
    public:
      explicit AcceptedMessageDecoder(const codec::DecoderHints& hints):
        PaxosMessageDecoder<rpc::AcceptedMessage>(hints){}
      AcceptedMessageDecoder(const AcceptedMessageDecoder& other) = default;
      ~AcceptedMessageDecoder() override = default;
      bool Decode(const BufferPtr& buff, rpc::AcceptedMessage& result) const override;
      AcceptedMessageDecoder& operator=(const AcceptedMessageDecoder& other) = default;
    };
  }

  namespace rpc{
    class AcceptedMessage : public PaxosMessage{
     public:
      AcceptedMessage():
        PaxosMessage(){}
      explicit AcceptedMessage(const Proposal& proposal):
        PaxosMessage(proposal){}
      ~AcceptedMessage() override = default;

      Type type() const override{
        return Type::kAcceptedMessage;
      }

      int64_t GetBufferSize() const override{
        codec::AcceptedMessageEncoder encoder((*this), codec::kDefaultEncoderFlags);
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        codec::AcceptedMessageEncoder encoder((*this), codec::kDefaultEncoderFlags);
        return encoder.Encode(buff);
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "AcceptedMessage(" << proposal() << ")";
        return ss.str();
      }

      AcceptedMessage& operator=(const AcceptedMessage& other) = default;

      static inline AcceptedMessagePtr
      NewInstance(const Proposal& proposal){
        return std::make_shared<AcceptedMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, AcceptedMessage& result, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        codec::AcceptedMessageDecoder decoder(hints);
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