#ifndef TOKEN_RPC_MESSAGE_ACCEPTED_H
#define TOKEN_RPC_MESSAGE_ACCEPTED_H

#include "rpc/rpc_message_paxos.h"

namespace token{
  namespace rpc{
    class AcceptedMessage : public PaxosMessage{
    public:
      class Builder : public PaxosMessageBuilderBase<AcceptedMessage>{};
     public:
      AcceptedMessage():
        PaxosMessage(){}
      explicit AcceptedMessage(RawProposal raw):
        PaxosMessage(std::move(raw)){}
      explicit AcceptedMessage(const BufferPtr& data):
        PaxosMessage(data){}
      ~AcceptedMessage() override = default;

      Type type() const override{
        return Type::kAcceptedMessage;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "AcceptedMessage(";
        ss << "proposal=" << height();
        ss << ")";
        return ss.str();
      }

      AcceptedMessage& operator=(const AcceptedMessage& other) = default;

      static inline AcceptedMessagePtr
      NewInstance(const ProposalPtr& proposal){
        return std::make_shared<AcceptedMessage>(proposal->raw());
      }

      static inline AcceptedMessagePtr
      Decode(const internal::BufferPtr& data){
        return std::make_shared<AcceptedMessage>(data);
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