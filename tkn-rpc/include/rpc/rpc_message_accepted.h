#ifndef TOKEN_RPC_MESSAGE_ACCEPTED_H
#define TOKEN_RPC_MESSAGE_ACCEPTED_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class AcceptedMessage : public PaxosMessage{
    public:
      class Encoder : public codec::PaxosMessageEncoder<rpc::AcceptedMessage>{
      public:
        explicit Encoder(const AcceptedMessage* value, const codec::EncoderFlags& flags):
          codec::PaxosMessageEncoder<rpc::AcceptedMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;
        Encoder& operator=(const Encoder& other) = default;
      };

    class Decoder : public codec::PaxosMessageDecoder<AcceptedMessage>{
      public:
        explicit Decoder(const codec::DecoderHints& hints):
          PaxosMessageDecoder<AcceptedMessage>(hints){}
        ~Decoder() override = default;
        AcceptedMessage* Decode(const BufferPtr& data) const override;
      };
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
        Encoder encoder(this, codec::kDefaultEncoderFlags);
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        Encoder encoder(this, codec::kDefaultEncoderFlags);
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

      static inline AcceptedMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);

        AcceptedMessage* value = nullptr;
        if(!(value = decoder.Decode(data))){
          DLOG(FATAL) << "cannot decode AcceptedMessage from buffer.";
          return nullptr;
        }
        return std::shared_ptr<AcceptedMessage>(value);
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