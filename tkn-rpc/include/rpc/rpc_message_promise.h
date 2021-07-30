#ifndef TOKEN_RPC_MESSAGE_PROMISE_H
#define TOKEN_RPC_MESSAGE_PROMISE_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class PromiseMessage : public PaxosMessage{
    public:
    class Encoder : public codec::PaxosMessageEncoder<rpc::PromiseMessage>{
      public:
        explicit Encoder(const PromiseMessage* value, const codec::EncoderFlags& flags):
          PaxosMessageEncoder<rpc::PromiseMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;
        Encoder& operator=(const Encoder& other) = default;
      };

    class Decoder : public codec::PaxosMessageDecoder<rpc::PromiseMessage>{
      public:
        explicit Decoder(const codec::DecoderHints& hints):
          PaxosMessageDecoder<rpc::PromiseMessage>(hints){}
        ~Decoder() override = default;
        PromiseMessage* Decode(const BufferPtr& data) const override;
      };
     public:
      PromiseMessage():
        PaxosMessage(){}
      explicit PromiseMessage(const Proposal& proposal):
        PaxosMessage(proposal){}
      PromiseMessage(const PromiseMessage& other) = default;
      ~PromiseMessage() override = default;

      int64_t GetBufferSize() const override{
        Encoder encoder(this, GetDefaultMessageEncoderFlags());
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        Encoder encoder(this, GetDefaultMessageEncoderFlags());
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

      static inline PromiseMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);

        PromiseMessage* value = nullptr;
        if(!(value = decoder.Decode(data))){
          DLOG(FATAL) << "cannot decode PromiseMessage from buffer.";
          return nullptr;
        }
        return std::shared_ptr<PromiseMessage>(value);
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