#ifndef TOKEN_RPC_MESSAGE_PAXOS_H
#define TOKEN_RPC_MESSAGE_PAXOS_H

#include "consensus/proposal.h"

namespace token{
#define DEFINE_PAXOS_MESSAGE_CONSTRUCTORS(Name) \
  DEFINE_RPC_MESSAGE_CONSTRUCTORS(Name)         \
  static inline Name##MessagePtr NewInstance(const RawProposal& proposal){ return std::make_shared<Name##Message>(proposal); } \
  static inline Name##MessagePtr NewInstance(const ProposalPtr& proposal){ return NewInstance(proposal->raw()); }

#define DEFINE_PAXOS_MESSAGE(Name) \
  DEFINE_RPC_MESSAGE(Name)         \
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

    PaxosMessage(const RawProposal& proposal):
      RpcMessage(),
      raw_(proposal){}
    PaxosMessage(const BufferPtr& buff):
      RpcMessage(),
      raw_(buff->GetTimestamp(), buff->GetUUID(), buff->GetUUID(), BlockHeader(buff)){}

    int64_t GetMessageSize() const{
      return raw_.GetBufferSize();
    }

    bool WriteMessage(const BufferPtr& buff) const{
      return raw_.Encode(buff);
    }
   public:
    virtual ~PaxosMessage() = default;

    RawProposal& GetProposal(){
      return raw_;
    }

    RawProposal GetProposal() const{
      return raw_;
    }
  };

  class PrepareMessage : public PaxosMessage{
   public:
    PrepareMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    PrepareMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~PrepareMessage(){}

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsPrepareMessage())
        return false;
      return true;//TODO: fix
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "PrepareMessage(" << GetProposal() << ")";
      return ss.str();
    }

    DEFINE_PAXOS_MESSAGE(Prepare);
  };

  class PromiseMessage : public PaxosMessage{
   public:
    PromiseMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    PromiseMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~PromiseMessage(){}

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsPromiseMessage()){
        return false;
      }
      return true; //TODO: fix
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "PromiseMessage(" << GetProposal() << ")";
      return ss.str();
    }

    DEFINE_PAXOS_MESSAGE(Promise);
  };

  class CommitMessage : public PaxosMessage{
   public:
    CommitMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    CommitMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~CommitMessage(){}

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsCommitMessage())
        return false;
      return true; //TODO: fix
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "CommitMessage(" << GetProposal() << ")";
      return ss.str();
    }

    DEFINE_PAXOS_MESSAGE(Commit);
  };

  class AcceptedMessage : public PaxosMessage{
   public:
    AcceptedMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    AcceptedMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~AcceptedMessage(){}

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsAcceptedMessage()){
        return false;
      }
      return true; //TODO: fix
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "AcceptedMessage(" << GetProposal() << ")";
      return ss.str();
    }

    DEFINE_PAXOS_MESSAGE(Accepted);
  };

  class RejectedMessage : public PaxosMessage{
   public:
    RejectedMessage(const RawProposal& proposal):
      PaxosMessage(proposal){}
    RejectedMessage(const BufferPtr& buff):
      PaxosMessage(buff){}
    ~RejectedMessage(){}

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsRejectedMessage())
        return false;
      return true; //TODO: fix
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "RejectedMessage(" << GetProposal() << ")";
      return ss.str();
    }

    DEFINE_PAXOS_MESSAGE(Rejected);
  };
}

#undef DEFINE_PAXOS_MESSAGE
#undef DEFINE_PAXOS_MESSAGE_CONSTRUCTORS

#endif//TOKEN_RPC_MESSAGE_PAXOS_H