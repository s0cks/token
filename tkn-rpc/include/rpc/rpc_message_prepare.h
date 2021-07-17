#ifndef TOKEN_RPC_MESSAGE_PREPARE_H
#define TOKEN_RPC_MESSAGE_PREPARE_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class PrepareMessage;
  }

  namespace codec{
    class PrepareMessageEncoder : public PaxosMessageEncoder<rpc::PrepareMessage>{
    public:
      PrepareMessageEncoder(const rpc::PrepareMessage& value, const EncoderFlags& flags):
        PaxosMessageEncoder<rpc::PrepareMessage>(value, flags){}
      PrepareMessageEncoder(const PrepareMessageEncoder& rhs) = default;
      ~PrepareMessageEncoder() override = default;
      PrepareMessageEncoder& operator=(const PrepareMessageEncoder& rhs) = default;
    };

    class PrepareMessageDecoder : public PaxosMessageDecoder<rpc::PrepareMessage>{
    public:
      explicit PrepareMessageDecoder(const DecoderHints& hints):
        PaxosMessageDecoder<rpc::PrepareMessage>(hints){}
      PrepareMessageDecoder(const PrepareMessageDecoder& rhs) = default;
      ~PrepareMessageDecoder() override = default;
      bool Decode(const BufferPtr& buff, rpc::PrepareMessage& result) const override;
      PrepareMessageDecoder& operator=(const PrepareMessageDecoder& rhs) = default;
    };
  }

  namespace rpc{
    class PrepareMessage : public PaxosMessage{
     public:
      PrepareMessage():
        PaxosMessage(){}
      explicit PrepareMessage(const Proposal& proposal):
        PaxosMessage(proposal){}
      PrepareMessage(const PrepareMessage& other) = default;
      ~PrepareMessage() override = default;

      Type type() const override{
        return Type::kPrepareMessage;
      }

      int64_t GetBufferSize() const override{
        codec::PrepareMessageEncoder encoder((*this), GetDefaultMessageEncoderFlags());
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buffer) const override{
        codec::PrepareMessageEncoder encoder((*this), GetDefaultMessageEncoderFlags());
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
      NewInstance(const Proposal& proposal){
        return std::make_shared<PrepareMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, PrepareMessage& result, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        codec::PrepareMessageDecoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline PrepareMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
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