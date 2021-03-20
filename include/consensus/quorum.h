#ifndef TOKEN_QUORUM_H
#define TOKEN_QUORUM_H

#include "atomic/relaxed_atomic.h"
#include "consensus/raw_proposal.h"
#include "consensus/proposal_phase.h"

namespace token{
  class Quorum;
  class QuorumResult{
    friend class Phase1Quorum;
    friend class Phase2Quorum;
   public:
    enum Status{
      kUnknown = 0,
      kAccepted,
      kRejected,
    };
   private:
    Quorum* quorum_;
    Status status_;
    int16_t total_;
    int16_t required_;
    int16_t accepted_;
    int16_t rejected_;

    QuorumResult(Quorum* quorum,
                 const Status& status,
                 const int16_t& total,
                 const int16_t& required,
                 const int16_t& accepted,
                 const int16_t& rejected):
      quorum_(quorum),
      status_(status),
      total_(total),
      required_(required),
      accepted_(accepted),
      rejected_(rejected){}
   public:
    QuorumResult(const QuorumResult& result):
      quorum_(result.quorum_),
      status_(result.status_),
      total_(result.total_),
      required_(result.required_),
      accepted_(result.accepted_),
      rejected_(result.required_){}
    ~QuorumResult() = default;

    Quorum* GetQuorum() const{
      return quorum_;
    }

    Status GetStatus() const{
      return status_;
    }

    int16_t GetRequiredVotes() const{
      return required_;
    }

    int16_t GetAcceptedVotes() const{
      return accepted_;
    }

    int16_t GetRejectedVotes() const{
      return rejected_;
    }

    int16_t GetTotalVotes() const{
      return accepted_ + rejected_;
    }

    double GetPercentageAccepted() const{
      return (GetAcceptedVotes() / GetTotalVotes()) * 100.0;
    }

    double GetPercentageRejected() const{
      return (GetRejectedVotes() / GetTotalVotes()) * 100.0;
    }

    void operator=(const QuorumResult& result){
      quorum_ = result.quorum_;
      status_ = result.status_;
      total_ = result.total_;
      required_ = result.required_;
      accepted_ = result.accepted_;
      rejected_ = result.rejected_;
    }
  };

  class Quorum{
   protected:
    ProposalPhase phase_;
    RawProposal proposal_;
    int16_t required_votes_;

    Quorum(uv_loop_t* loop, const ProposalPhase& phase, const RawProposal& proposal, const int16_t& required_votes):
      phase_(phase),
      proposal_(proposal),
      required_votes_(required_votes){}
   public:
    virtual ~Quorum() = default;

    ProposalPhase GetPhase() const{
      return phase_;
    }

    RawProposal& GetProposal(){
      return proposal_;
    }

    RawProposal GetProposal() const{
      return proposal_;
    }

    int16_t GetRequiredVotes() const{
      return required_votes_;
    }

    virtual int16_t GetNumberOfVotes() const = 0;
    virtual QuorumResult GetResult() = 0;

    void WaitForRequiredVotes() const{
      DLOG(INFO) << "waiting for " << GetRequiredVotes() << " votes....";
      while(GetNumberOfVotes() < GetRequiredVotes()); // spin
    }
  };

  class Phase1Quorum : public Quorum{
    friend class Proposal;
    friend class PeerSession;
   public:
    static const int64_t kPhase1TimeoutMilliseconds = 60 * 1000;
   protected:
    uv_timer_t timeout_;
    RelaxedAtomic<int16_t> promised_;
    RelaxedAtomic<int16_t> rejected_;

    void PromiseProposal(const UUID& uuid){
      DLOG(INFO) << uuid << " promised proposal: " << GetProposal();
      promised_ += 1;
    }

    void RejectProposal(const UUID& uuid){
      DLOG(INFO) << uuid << " rejected proposal: " << GetProposal();
      rejected_ += 1;
    }

    static void OnTimeout(uv_timer_t* handle){
      DLOG(ERROR) << "proposal phase 1 timeout.";
    }
   public:
    Phase1Quorum(uv_loop_t* loop, const RawProposal& proposal, const int16_t& required_votes):
      Quorum(loop, ProposalPhase::kPreparePhase, proposal, required_votes),
      timeout_(),
      promised_(0),
      rejected_(0){
      timeout_.data = this;
      CHECK_UVRESULT(uv_timer_init(loop, &timeout_), LOG(ERROR), "cannot initialize phase 1 quorum timer");
    }
    ~Phase1Quorum() = default;

    int16_t GetPromised() const{
      return promised_;
    }

    int16_t GetRejected() const{
      return rejected_;
    }

    int16_t GetNumberOfVotes() const{
      return promised_ + rejected_;
    }

    bool StartTimer(){
      DLOG(INFO) << "stopping phase 1 timer....";
      VERIFY_UVRESULT(uv_timer_start(&timeout_, &OnTimeout, kPhase1TimeoutMilliseconds, 0), LOG(ERROR), "cannot start phase 1 quorum timer");
      return true;
    }

    bool StopTimer(){
      DLOG(INFO) << "stopping phase 1 timer....";
      VERIFY_UVRESULT(uv_timer_stop(&timeout_), LOG(ERROR), "cannot stop phase 1 quorum timer");
      return true;
    }

    QuorumResult GetResult(){
      int16_t total_votes = promised_ + rejected_;
      int16_t required = (required_votes_ / 2) + 1;
      if(GetPromised() >= required){
        return QuorumResult(this, QuorumResult::kAccepted, total_votes, required, promised_, rejected_);
      } else if(GetRejected() >= required){
        return QuorumResult(this, QuorumResult::kRejected, total_votes, required, promised_, rejected_);
      }
      return QuorumResult(this, QuorumResult::kUnknown, total_votes, required, promised_, rejected_);
    }
  };

  class Phase2Quorum : public Quorum{
    friend class PeerSession;
   public:
    static const int64_t kPhase2TimeoutMilliseconds = 60 * 1000;
   protected:
    uv_timer_t timeout_;
    RelaxedAtomic<int16_t> accepted_;
    RelaxedAtomic<int16_t> rejected_;

    void AcceptProposal(const UUID& uuid){
      DLOG(INFO) << uuid.ToStringAbbreviated() << " accepted proposal: " << GetProposal();
      accepted_ += 1;
    }

    void RejectProposal(const UUID& uuid){
      DLOG(INFO) << uuid << " rejected proposal: " << GetProposal();
      rejected_ += 1;
    }

    static void OnTimeout(uv_timer_t* handle){
      LOG(ERROR) << "proposal phase 2 timeout.";
    }
   public:
    Phase2Quorum(uv_loop_t* loop, const RawProposal& proposal, const int16_t& required_votes):
      Quorum(loop, ProposalPhase::kCommitPhase, proposal, required_votes),
      timeout_(),
      accepted_(0),
      rejected_(0){
      timeout_.data = this;
      CHECK_UVRESULT(uv_timer_init(loop, &timeout_), LOG(ERROR), "cannot initialize phase 2 timer");
    }
    ~Phase2Quorum() = default;

    int16_t GetAccepted() const{
      return accepted_;
    }

    int16_t GetRejected() const{
      return rejected_;
    }

    int16_t GetNumberOfVotes() const{
      return accepted_ + rejected_;
    }

    bool StartTimer(){
      DLOG(INFO) << "starting phase 2 timer....";
      VERIFY_UVRESULT(uv_timer_start(&timeout_, &OnTimeout, kPhase2TimeoutMilliseconds, 0), LOG(ERROR), "cannot start phase 2 timer");
      return true;
    }

    bool StopTimer(){
      DLOG(INFO) << "stopping phase 1 timer....";
      VERIFY_UVRESULT(uv_timer_stop(&timeout_), LOG(ERROR), "cannot stop phase 2 timer");
      return true;
    }

    QuorumResult GetResult(){
      int16_t total_votes = accepted_ + rejected_;
      int16_t required = (required_votes_ / 2) + 1;
      if(GetAccepted() >= required){
        return QuorumResult(this, QuorumResult::kAccepted, total_votes, required, accepted_, rejected_);
      } else if(GetRejected() >= required){
        return QuorumResult(this, QuorumResult::kRejected, total_votes, required, accepted_, rejected_);
      }
      return QuorumResult(this, QuorumResult::kUnknown, total_votes, required, accepted_, rejected_);
    }
  };

  static std::ostream& operator<<(std::ostream& stream, const QuorumResult& result){
    RawProposal& proposal = result.GetQuorum()->GetProposal();
    switch(result.GetStatus()){
      case QuorumResult::kAccepted:
        return stream << "proposal " << proposal << " was accepted by " << result.GetAcceptedVotes() << " peers (" << result.GetPercentageAccepted() << "%)";
      case QuorumResult::kRejected:
        return stream << "proposal " << proposal << " was rejected by " << result.GetRejectedVotes() << " peers (" << result.GetPercentageRejected() << "%)";
      default:
        return stream << "proposal " << proposal << " reached an unknown consensus: " << result.GetStatus();
    }
  }
}

#endif//TOKEN_QUORUM_H