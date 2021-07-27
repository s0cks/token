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

    class Decoder : public codec::PaxosMessageDecoder<rpc::PrepareMessage>{
      public:
        explicit Decoder(const codec::DecoderHints& hints):
          PaxosMessageDecoder<rpc::PrepareMessage>(hints){}
        Decoder(const Decoder& rhs) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, rpc::PrepareMessage& result) const override;
        Decoder& operator=(const Decoder& rhs) = default;
      };
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

      PrepareMessage& operator=(const PrepareMessage& rhs){
        if(this == &rhs)
          return *this;
        delete proposal_;
        proposal_ = new Proposal(*rhs.proposal_);
        return *this;
      }

      friend std::ostream& operator<<(std::ostream& stream, const PrepareMessage& msg){
        return stream << msg.ToString();
      }

      static inline PrepareMessagePtr
      NewInstance(const Proposal& proposal){
        return std::make_shared<PrepareMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, PrepareMessage& result, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        Decoder decoder(hints);
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