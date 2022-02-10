#ifndef TKN_PROPOSAL_PHASE_H
#define TKN_PROPOSAL_PHASE_H

#include "../../../Sources/token/proposal.h"

namespace token{
#define FOR_EACH_PROPOSAL_PHASE(V) \
  V(Queued)                        \
  V(Prepare)               \
  V(Commit)                \
  V(Quorum)

  class Runtime;
  class ProposalState : public ProposalEventListener{
    friend class Runtime;
  public:
    enum Phase{
#define DEFINE_PHASE(Name) k##Name##Phase,
      FOR_EACH_PROPOSAL_PHASE(DEFINE_PHASE)
#undef DEFINE_PHASE
    };

    friend std::ostream&
    operator<<(std::ostream& stream, const Phase& val){
      switch(val){
#define DEFINE_TOSTRING(Name) \
        case Phase::k##Name##Phase: return stream << #Name;
        FOR_EACH_PROPOSAL_PHASE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "unknown";
      }
    }
  protected:
    Runtime* runtime_;
    atomic::RelaxedAtomic<Phase> phase_;
    atomic::RelaxedAtomic<bool> active_;
    atomic::RelaxedAtomic<uint64_t> height_;

    std::mutex mutex_;
    UUID proposal_id_;
    std::shared_ptr<Proposal> proposal_;
    UUID proposer_id_;
    node::Session* proposer_;

    DEFINE_PROPOSAL_EVENT_LISTENER;

    void SubscribeTo(EventBus& bus){
      bus.Subscribe("proposal.prepare", &on_prepare_);
      bus.Subscribe("proposal.commit", &on_commit_);
    }
  public:
    explicit ProposalState(Runtime* runtime);
    ~ProposalState() override = default;

    Phase GetPhase() const{
      return (Phase)phase_;
    }

    bool WasProposedBy(const UUID& val){
      return GetProposerId() == val;
    }

    bool IsActive() const{
      return (bool)active_;
    }

    void SetPhase(const Phase& phase){
      DLOG(INFO) << "setting proposal phase to: " << phase;
      phase_ = phase;
    }

    uint64_t GetHeight(){
      return (uint64_t)height_;
    }

    UUID GetProposerId(){
      std::lock_guard<std::mutex> guard(mutex_);
      return proposer_id_;
    }

    UUID GetProposalId(){
      std::lock_guard<std::mutex> guard(mutex_);
      return proposal_id_;
    }

    std::shared_ptr<Proposal> GetProposal(){
      std::lock_guard<std::mutex> guard(mutex_);
      return proposal_;
    }

    node::Session* GetProposer(){
      std::lock_guard<std::mutex> guard(mutex_);
      return proposer_;
    }

    bool CreateNewProposal(const UUID& proposer_id, const Hash& val, const UUID& proposal_id=UUID(), const Timestamp& timestamp=Clock::now(), node::Session* session=nullptr){
      if(IsActive()){
        DLOG(WARNING) << "there is already an active proposal.";
        return false;
      }

      std::lock_guard<std::mutex> guard(mutex_);
      active_ = true;
      height_ += 1;
      proposal_id_ = proposal_id;
      proposer_id_ = proposer_id;
      proposer_ = session;
      proposal_ = Proposal::NewInstance(timestamp, proposal_id_, proposer_id_, GetHeight(), val);
      return true;
    }

    bool SetCurrentProposal(const std::shared_ptr<Proposal>& proposal, node::Session* session=nullptr){
      if(IsActive()){
        DLOG(WARNING) << "there is already an active proposal.";
        return false;
      }

      std::lock_guard<std::mutex> guard(mutex_);
      active_ = true;
      height_ = proposal->height();
      proposal_id_ = proposal->proposal_id();
      proposer_id_ = proposal->proposer_id();
      proposer_ = session;
      proposal_ = proposal;
      return true;
    }

    bool Clear(){
      if(!IsActive()){
        DLOG(WARNING) << "there is no active proposal currently.";
        return false;
      }

      //TODO: this is a terrible function
      std::lock_guard<std::mutex> guard(mutex_);
      active_ = true;
      height_ = 0;
      proposal_id_ = UUID();
      proposer_id_ = UUID();
      proposer_ = nullptr;
      proposal_ = nullptr;
      return true;
    }
  };
}

#endif//TKN_PROPOSAL_PHASE_H