#ifndef TOKEN_RPC_MESSAGE_PROMISE_H
#define TOKEN_RPC_MESSAGE_PROMISE_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class PromiseMessage : public PaxosMessage{
    public:
      class Builder : public PaxosMessageBuilderBase<PromiseMessage>{};
     public:
      PromiseMessage():
        PaxosMessage(){}
      explicit PromiseMessage(RawProposal raw):
        PaxosMessage(std::move(raw)){}
      explicit PromiseMessage(const BufferPtr& data, const uint64_t& msize):
        PaxosMessage(data, msize){}
      ~PromiseMessage() override = default;

      Type type() const override{
        return Type::kPromiseMessage;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "PromiseMessage(";
        ss << "proposal=" << height();
        ss << ")";
        return ss.str();
      }

      PromiseMessage& operator=(const PromiseMessage& other) = default;

      friend std::ostream& operator<<(std::ostream& stream, const PromiseMessage& msg){
        return stream << msg.ToString();
      }

      static inline PromiseMessagePtr
      NewInstance(const ProposalPtr& proposal){
        return std::make_shared<PromiseMessage>(proposal->raw());
      }

      static inline PromiseMessagePtr
      Decode(const internal::BufferPtr& data, const uint64_t& msize){
        return std::make_shared<PromiseMessage>(data, msize);
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