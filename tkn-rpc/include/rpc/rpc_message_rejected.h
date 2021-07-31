#ifndef TOKEN_RPC_MESSAGE_REJECTED_H
#define TOKEN_RPC_MESSAGE_REJECTED_H

#include <utility>

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class RejectedMessage : public PaxosMessage{
    public:
      class Builder : public PaxosMessageBuilderBase<RejectedMessage>{};
     public:
      RejectedMessage() = default;
      explicit RejectedMessage(RawProposal raw):
        PaxosMessage(std::move(raw)){}
      explicit RejectedMessage(const BufferPtr& data):
        PaxosMessage(data){}
      ~RejectedMessage() override = default;

      Type type() const override{
        return Type::kRejectedMessage;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "RejectedMessage(";
        ss << "proposal=" << height();
        ss << ")";
        return ss.str();
      }

      RejectedMessage& operator=(const RejectedMessage& other) = default;

      static inline RejectedMessagePtr
      NewInstance(const ProposalPtr& proposal){
        return std::make_shared<RejectedMessage>(proposal->raw());
      }

      static inline RejectedMessagePtr
      Decode(const BufferPtr& data){
        return std::make_shared<RejectedMessage>(data);
      }
    };

    static inline std::ostream&
    operator<<(std::ostream& stream, const RejectedMessagePtr& msg){
      return stream << msg->ToString();
    }

    static inline MessageList&
    operator<<(MessageList& list, const RejectedMessagePtr& msg){
      list.push_back(msg);
      return list;
    }
  }
}

#endif//TOKEN_RPC_MESSAGE_REJECTED_H