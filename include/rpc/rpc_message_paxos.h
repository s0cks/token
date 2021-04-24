#ifndef TOKEN_RPC_MESSAGE_PAXOS_H
#define TOKEN_RPC_MESSAGE_PAXOS_H

#include "proposal.h"

namespace token{
#define DEFINE_PAXOS_MESSAGE_CONSTRUCTORS(Name) \
  DEFINE_RPC_MESSAGE_CONSTRUCTORS(Name)         \
  static inline Name##MessagePtr NewInstance(const RawProposal& proposal){ return std::make_shared<Name##Message>(proposal); } \
  static inline Name##MessagePtr NewInstance(const ProposalPtr& proposal){ return NewInstance(proposal->raw()); }

#define DEFINE_PAXOS_MESSAGE_TYPE(Name) \
  DEFINE_RPC_MESSAGE_TYPE(Name)         \
  DEFINE_PAXOS_MESSAGE_CONSTRUCTORS(Name)

  class Proposal;
  class PaxosMessage : public RpcMessage{
   public:
    static inline int
    CompareProposal(const std::shared_ptr<PaxosMessage>& a, const std::shared_ptr<PaxosMessage>& b){
      if(a->GetProposal() < b->GetProposal()){
        return -1;
      } else if(a->GetProposal() > b->GetProposal()){
        return +1;
      }
      return 0;
    }
   protected:
    RawProposal raw_;

    explicit PaxosMessage(const RawProposal& proposal):
      RpcMessage(),
      raw_(proposal){}
    explicit PaxosMessage(const BufferPtr& buff):
      RpcMessage(),
      raw_(buff){}
   public:
    ~PaxosMessage() override = default;

    int64_t GetMessageSize() const override{
      return raw_.GetBufferSize();
    }

    bool WriteMessage(const BufferPtr& buff) const override{
      return raw_.Write(buff);
    }

    RawProposal& GetProposal(){
      return raw_;
    }

    RawProposal GetProposal() const{
      return raw_;
    }
  };

  class PrepareMessage : public PaxosMessage{
   public:
    explicit PrepareMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    explicit PrepareMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~PrepareMessage() override = default;

    DEFINE_PAXOS_MESSAGE_TYPE(Prepare);

    bool Equals(const RpcMessagePtr& obj) const override{
      if(!obj->IsPrepareMessage())
        return false;
      return GetProposal() == std::static_pointer_cast<PrepareMessage>(obj)->GetProposal();
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "PrepareMessage(" << GetProposal() << ")";
      return ss.str();
    }
  };

  class PromiseMessage : public PaxosMessage{
   public:
    explicit PromiseMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    explicit PromiseMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~PromiseMessage() override = default;

    DEFINE_PAXOS_MESSAGE_TYPE(Promise);

    bool Equals(const RpcMessagePtr& obj) const override{
      if(!obj->IsPromiseMessage())
        return false;
      return GetProposal() == std::static_pointer_cast<PromiseMessage>(obj)->GetProposal();
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "PromiseMessage(" << GetProposal() << ")";
      return ss.str();
    }
  };

  class CommitMessage : public PaxosMessage{
   public:
    explicit CommitMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    explicit CommitMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~CommitMessage() override = default;

    DEFINE_PAXOS_MESSAGE_TYPE(Commit);

    bool Equals(const RpcMessagePtr& obj) const override{
      if(!obj->IsCommitMessage())
        return false;
      return GetProposal() == std::static_pointer_cast<CommitMessage>(obj)->GetProposal();
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "CommitMessage(" << GetProposal() << ")";
      return ss.str();
    }
  };

  class AcceptsMessage : public PaxosMessage{
   public:
    explicit AcceptsMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    explicit AcceptsMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~AcceptsMessage() override = default;

    DEFINE_PAXOS_MESSAGE_TYPE(Accepts);

    bool Equals(const RpcMessagePtr& obj) const override{
      if(!obj->IsAcceptsMessage())
        return false;
      return GetProposal() == std::static_pointer_cast<AcceptsMessage>(obj)->GetProposal();
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "AcceptsMessage(";
      ss << "proposal=" << raw_;
      ss << ")";
      return ss.str();
    }
  };

  class AcceptedMessage : public PaxosMessage{
   public:
    explicit AcceptedMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    explicit AcceptedMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~AcceptedMessage() override = default;

    DEFINE_PAXOS_MESSAGE_TYPE(Accepted);

    bool Equals(const RpcMessagePtr& obj) const override{
      if(!obj->IsAcceptedMessage())
        return false;
      return GetProposal() == std::static_pointer_cast<AcceptedMessage>(obj)->GetProposal();
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "AcceptedMessage(" << GetProposal() << ")";
      return ss.str();
    }
  };

  class RejectsMessage : public PaxosMessage{
   public:
    explicit RejectsMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    explicit RejectsMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~RejectsMessage() override = default;

    DEFINE_PAXOS_MESSAGE_TYPE(Rejects);

    bool Equals(const RpcMessagePtr& obj) const override{
      if(!obj->IsRejectsMessage())
        return false;
      return GetProposal() == std::static_pointer_cast<RejectsMessage>(obj)->GetProposal();
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "RejectsMessage(";
      ss << "proposal=" << raw_;
      ss << ")";
      return ss.str();
    }
  };

  class RejectedMessage : public PaxosMessage{
   public:
    explicit RejectedMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    explicit RejectedMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~RejectedMessage() override = default;

    DEFINE_PAXOS_MESSAGE_TYPE(Rejected);

    bool Equals(const RpcMessagePtr& obj) const override{
      if(!obj->IsRejectedMessage())
        return false;
      return GetProposal() == std::static_pointer_cast<RejectedMessage>(obj)->GetProposal();
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "RejectedMessage(" << GetProposal() << ")";
      return ss.str();
    }
  };
}

#undef DEFINE_PAXOS_MESSAGE_TYPE
#undef DEFINE_PAXOS_MESSAGE_CONSTRUCTORS

#endif//TOKEN_RPC_MESSAGE_PAXOS_H