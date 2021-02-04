#ifndef TOKEN_RPC_MESSAGE_PAXOS_H
#define TOKEN_RPC_MESSAGE_PAXOS_H

#include "rpc/rpc_message.h"
#include "consensus/proposal.h"

namespace token{
  class Proposal;
  class PaxosMessage : public RpcMessage{
   protected:
    RawProposal raw_;

    PaxosMessage(ProposalPtr proposal):
      RpcMessage(),
      raw_(proposal->GetRaw()){}
    PaxosMessage(const BufferPtr& buff):
      RpcMessage(),
      raw_(buff){}

    int64_t GetMessageSize() const{
      return RawProposal::GetSize();
    }

    bool WriteMessage(const BufferPtr& buff) const{
      return raw_.Encode(buff);
    }
   public:
    virtual ~PaxosMessage() = default;

    RawProposal GetRaw() const{
      return raw_;
    }

    ProposalPtr GetProposal() const;

    bool ProposalEquals(const std::shared_ptr<PaxosMessage>& msg) const{
      LOG(INFO) << raw_ << " <=> " <<msg->raw_;
      return raw_ == msg->raw_;
    }
  };

  class PrepareMessage : public PaxosMessage{
   public:
    PrepareMessage(const ProposalPtr& proposal):
      PaxosMessage(proposal){}
    PrepareMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~PrepareMessage(){}

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsPrepareMessage())
        return false;
      return PaxosMessage::ProposalEquals(std::static_pointer_cast<PaxosMessage>(obj));
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "PrepareMessage(" << GetProposal()->ToString() << ")";
      return ss.str();
    }

    DEFINE_RPC_MESSAGE(Prepare);

    static inline RpcMessagePtr
    NewInstance(const ProposalPtr& proposal){
      return std::make_shared<PrepareMessage>(proposal);
    }

    static inline RpcMessagePtr
    NewInstance(const BufferPtr& buff){
      return std::make_shared<PrepareMessage>(buff);
    }
  };

  class PromiseMessage : public PaxosMessage{
   public:
    PromiseMessage(ProposalPtr proposal):
      PaxosMessage(proposal){}
    PromiseMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~PromiseMessage(){}

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsPromiseMessage()){
        return false;
      }
      return PaxosMessage::ProposalEquals(std::static_pointer_cast<PaxosMessage>(obj));
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "ProposalMessage(" << GetProposal()->ToString() << ")";
      return ss.str();
    }

    DEFINE_RPC_MESSAGE(Promise);

    static inline RpcMessagePtr
    NewInstance(const ProposalPtr& proposal){
      return std::make_shared<PromiseMessage>(proposal);
    }

    static inline RpcMessagePtr
    NewInstance(const BufferPtr& buff){
      return std::make_shared<PromiseMessage>(buff);
    }
  };

  class CommitMessage : public PaxosMessage{
   public:
    CommitMessage(ProposalPtr proposal):
      PaxosMessage(proposal){}
    CommitMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~CommitMessage(){}

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsCommitMessage())
        return false;
      return PaxosMessage::ProposalEquals(std::static_pointer_cast<PaxosMessage>(obj));
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "CommitMessage(" << GetProposal()->ToString() << ")";
      return ss.str();
    }

    DEFINE_RPC_MESSAGE(Commit);

    static inline RpcMessagePtr
    NewInstance(const ProposalPtr& proposal){
      return std::make_shared<CommitMessage>(proposal);
    }

    static inline RpcMessagePtr
    NewInstance(const BufferPtr& buff){
      return std::make_shared<CommitMessage>(buff);
    }
  };

  class AcceptedMessage : public PaxosMessage{
   public:
    AcceptedMessage(ProposalPtr proposal):
      PaxosMessage(proposal){}
    AcceptedMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~AcceptedMessage(){}

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsAcceptedMessage()){
        return false;
      }
      return PaxosMessage::ProposalEquals(std::static_pointer_cast<PaxosMessage>(obj));
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "AcceptedMessage(" << GetProposal()->ToString() << ")";
      return ss.str();
    }

    DEFINE_RPC_MESSAGE(Accepted);

    static inline RpcMessagePtr
    NewInstance(const ProposalPtr& proposal){
      return std::make_shared<AcceptedMessage>(proposal);
    }

    static inline RpcMessagePtr
    NewInstance(const BufferPtr& buff){
      return std::make_shared<AcceptedMessage>(buff);
    }
  };

  class RejectedMessage : public PaxosMessage{
   public:
    RejectedMessage(ProposalPtr proposal):
      PaxosMessage(proposal){}
    RejectedMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~RejectedMessage(){}

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsRejectedMessage())
        return false;
      return PaxosMessage::ProposalEquals(std::static_pointer_cast<PaxosMessage>(obj));
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "RejectedMessage(" << GetProposal()->ToString() << ")";
      return ss.str();
    }

    DEFINE_RPC_MESSAGE(Rejected);

    static inline RpcMessagePtr
    NewInstance(const ProposalPtr& proposal){
      return std::make_shared<RejectedMessage>(proposal);
    }

    static inline RpcMessagePtr
    NewInstance(const BufferPtr& buff){
      return std::make_shared<RejectedMessage>(buff);
    }
  };
}

#endif//TOKEN_RPC_MESSAGE_PAXOS_H