#ifndef TOKEN_RPC_MESSAGE_PREPARE_H
#define TOKEN_RPC_MESSAGE_PREPARE_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class PrepareMessage : public PaxosMessage{
    public:
    class Encoder : public codec::PaxosMessageEncoder<rpc::PrepareMessage>{
      public:
        Encoder(const rpc::PrepareMessage* value, const codec::EncoderFlags& flags):
          PaxosMessageEncoder<rpc::PrepareMessage>(value, flags){}
        Encoder(const Encoder& rhs) = default;
        ~Encoder() override = default;
        Encoder& operator=(const Encoder& rhs) = default;
      };

    class Decoder : public codec::PaxosMessageDecoder<PrepareMessage>{
      public:
        explicit Decoder(const codec::DecoderHints& hints):
          PaxosMessageDecoder<PrepareMessage>(hints){}
        ~Decoder() override = default;
        PrepareMessage* Decode(const BufferPtr& data) const override;
      };
     public:
      PrepareMessage():
        PaxosMessage(){}
      explicit PrepareMessage(const Proposal& proposal):
        PaxosMessage(proposal){}
      ~PrepareMessage() override = default;

      Type type() const override{
        return Type::kPrepareMessage;
      }

      int64_t GetBufferSize() const override{
        Encoder encoder(this, GetDefaultMessageEncoderFlags());
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buffer) const override{
        Encoder encoder(this, GetDefaultMessageEncoderFlags());
        return encoder.Encode(buffer);
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "PrepareMessage(";
        ss << "proposal=" << ( proposal_ ? proposal_->height() : 0);
        ss << ")";
        return ss.str();
      }

      friend std::ostream& operator<<(std::ostream& stream, const PrepareMessage& msg){
        return stream << msg.ToString();
      }

      static inline PrepareMessagePtr
      NewInstance(const Proposal& proposal){
        return std::make_shared<PrepareMessage>(proposal);
      }

      static inline PrepareMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);

        PrepareMessage* value = nullptr;
        if(!(value = decoder.Decode(data))){
          DLOG(FATAL) << "cannot decode PrepareMessage from buffer.";
          return nullptr;
        }
        return std::shared_ptr<PrepareMessage>(value);
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