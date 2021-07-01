#ifndef TOKEN_RPC_MESSAGE_COMMIT_H
#define TOKEN_RPC_MESSAGE_COMMIT_H

#include "network/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class CommitMessage : public PaxosMessage{
     public:
      class Encoder : public PaxosMessageEncoder<CommitMessage>{
       public:
        Encoder(const CommitMessage& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
            PaxosMessageEncoder<CommitMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;

        int64_t GetBufferSize() const override{
          return PaxosMessageEncoder<CommitMessage>::GetBufferSize();
        }

        bool Encode(const BufferPtr& buff) const override{
          return PaxosMessageEncoder<CommitMessage>::Encode(buff);
        }

        Encoder& operator=(const Encoder& other) = default;
      };

      class Decoder : public PaxosMessageDecoder<CommitMessage>{
       public:
        Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
            PaxosMessageDecoder<CommitMessage>(hints){}
            Decoder(const Decoder& other) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, CommitMessage& result) const override;
        Decoder& operator=(const Decoder& other) = default;
      };
     public:
      CommitMessage():
        PaxosMessage(){}
      CommitMessage(const RawProposal& proposal):
        PaxosMessage(proposal){}
      ~CommitMessage() override = default;

      Type type() const override{
        return Type::kCommitMessage;
      }

      int64_t GetBufferSize() const override{
        Encoder encoder((*this));
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        Encoder encoder((*this));
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
      NewInstance(const RawProposal& proposal){
        return std::make_shared<CommitMessage>(proposal);
      }

      static inline bool
      Decode(const BufferPtr& buff, CommitMessage& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline CommitMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        CommitMessage instance;
        if(!Decode(buff, instance, hints)){
          DLOG(ERROR) << "cannot decode CommitMessage.";
          return nullptr;
        }
        return std::make_shared<CommitMessage>(instance);
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