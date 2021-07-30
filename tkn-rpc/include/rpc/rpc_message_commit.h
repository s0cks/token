#ifndef TOKEN_RPC_MESSAGE_COMMIT_H
#define TOKEN_RPC_MESSAGE_COMMIT_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class CommitMessage : public PaxosMessage{
    public:
      class Encoder : public codec::PaxosMessageEncoder<rpc::CommitMessage>{
      public:
        Encoder(const CommitMessage* value, const codec::EncoderFlags& flags):
          PaxosMessageEncoder<rpc::CommitMessage>(value, flags){}
        Encoder(const Encoder& rhs) = default;
        ~Encoder() override = default;
        Encoder& operator=(const Encoder& rhs) = default;
      };

      class Decoder : public codec::PaxosMessageDecoder<CommitMessage>{
      public:
        explicit Decoder(const codec::DecoderHints& hints):
          PaxosMessageDecoder<CommitMessage>(hints){}
        ~Decoder() override = default;
        CommitMessage* Decode(const BufferPtr& data) const override;
      };
     public:
      CommitMessage() = default;
      explicit CommitMessage(const Proposal& proposal):
        PaxosMessage(proposal){}
      ~CommitMessage() override = default;

      Type type() const override{
        return Type::kCommitMessage;
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
        ss << "CommitMessage(" << proposal() << ")";
        return ss.str();
      }

      CommitMessage& operator=(const CommitMessage& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const CommitMessage& msg){
        return stream << msg.ToString();
      }

      static inline CommitMessagePtr
      NewInstance(const Proposal& proposal){
        return std::make_shared<CommitMessage>(proposal);
      }

      static inline CommitMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);

        CommitMessage* value = nullptr;
        if(!(value = decoder.Decode(data))){
          DLOG(FATAL) << "cannot decode CommitMessage from buffer.";
          return nullptr;
        }
        return std::shared_ptr<CommitMessage>(value);
      }
    };

    static inline std::ostream&
    operator<<(std::ostream& stream, const CommitMessagePtr& msg){
      return stream << msg->ToString();
    }

    static inline MessageList&
    operator<<(MessageList& list, const CommitMessagePtr& msg){
      list.push_back(msg);
      return list;
    }
  }
}

#endif//TOKEN_RPC_MESSAGE_COMMIT_H