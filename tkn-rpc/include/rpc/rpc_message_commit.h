#ifndef TOKEN_RPC_MESSAGE_COMMIT_H
#define TOKEN_RPC_MESSAGE_COMMIT_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class CommitMessage : public PaxosMessage{
     public:
      CommitMessage() = default;
      explicit CommitMessage(RawProposal raw):
        PaxosMessage(std::move(raw)){}
      explicit CommitMessage(const BufferPtr& data, const uint64_t& msize):
        PaxosMessage(data, msize){}
      ~CommitMessage() override = default;

      Type type() const override{
        return Type::kCommitMessage;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "CommitMessage(";
        ss << "proposal=" << height();
        ss << ")";
        return ss.str();
      }

      CommitMessage& operator=(const CommitMessage& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const CommitMessage& msg){
        return stream << msg.ToString();
      }

      static inline CommitMessagePtr
      NewInstance(const ProposalPtr& proposal){
        return std::make_shared<CommitMessage>(proposal->raw());
      }

      static inline CommitMessagePtr
      Decode(const internal::BufferPtr& data, const uint64_t& msize){
        return std::make_shared<CommitMessage>(data, msize);
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