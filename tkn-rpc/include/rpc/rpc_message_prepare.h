#ifndef TOKEN_RPC_MESSAGE_PREPARE_H
#define TOKEN_RPC_MESSAGE_PREPARE_H

#include <utility>

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class PrepareMessage : public PaxosMessage{
    public:
      class Builder : public PaxosMessageBuilderBase<PrepareMessage>{};
     public:
      PrepareMessage():
        PaxosMessage(){}
      explicit PrepareMessage(RawProposal raw):
        PaxosMessage(std::move(raw)){}
      explicit PrepareMessage(const internal::BufferPtr& data, const uint64_t& msize):
        PaxosMessage(data, msize){}
      ~PrepareMessage() override = default;

      Type type() const override{
        return Type::kPrepareMessage;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "PrepareMessage(";
        ss << "proposal=" << height() << ", ";
        ss << ")";
        return ss.str();
      }

      friend std::ostream& operator<<(std::ostream& stream, const PrepareMessage& msg){
        return stream << msg.ToString();
      }

      static inline PrepareMessagePtr
      NewInstance(const ProposalPtr& proposal){
        return std::make_shared<PrepareMessage>(proposal->raw());
      }

      static inline PrepareMessagePtr
      Decode(const BufferPtr& data, const uint64_t& msize){
        return std::make_shared<PrepareMessage>(data, msize);
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