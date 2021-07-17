#ifndef TOKEN_RPC_MESSAGE_PROMISE_H
#define TOKEN_RPC_MESSAGE_PROMISE_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class PromiseMessage;
  }

  namespace codec{
    class PromiseMessageEncoder : public PaxosMessageEncoder<rpc::PromiseMessage>{
    public:
      explicit PromiseMessageEncoder(const rpc::PromiseMessage& value, const codec::EncoderFlags& flags):
        PaxosMessageEncoder<rpc::PromiseMessage>(value, flags){}
      PromiseMessageEncoder(const PromiseMessageEncoder& other) = default;
      ~PromiseMessageEncoder() override = default;
      PromiseMessageEncoder& operator=(const PromiseMessageEncoder& other) = default;
    };

    class PromiseMessageDecoder : public PaxosMessageDecoder<rpc::PromiseMessage>{
    public:
      explicit PromiseMessageDecoder(const codec::DecoderHints& hints):
        PaxosMessageDecoder<rpc::PromiseMessage>(hints){}
      PromiseMessageDecoder(const PromiseMessageDecoder& other) = default;
      ~PromiseMessageDecoder() override = default;
      bool Decode(const BufferPtr& buff, rpc::PromiseMessage& result) const override;
      PromiseMessageDecoder& operator=(const PromiseMessageDecoder& other) = default;
    };
  }

  namespace rpc{
    class PromiseMessage : public PaxosMessage{
     public:
      PromiseMessage():
        PaxosMessage(){}
      explicit PromiseMessage(const Proposal& proposal):
        PaxosMessage(proposal){}
      PromiseMessage(const PromiseMessage& other) = default;
      ~PromiseMessage() override = default;

      int64_t GetBufferSize() const override{
        codec::PromiseMessageEncoder encoder((*this), GetDefaultMessageEncoderFlags());
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        codec::PromiseMessageEncoder encoder((*this), GetDefaultMessageEncoderFlags());
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
      NewInstance(const Proposal& proposal){
        return std::make_shared<PromiseMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, PromiseMessage& result, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        codec::PromiseMessageDecoder decoder(hints);
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