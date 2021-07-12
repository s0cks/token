#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include <memory>
#include <utility>

#include "timestamp.h"
#include "raw_proposal.h"
#include "block_header.h"
#include "configuration.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  class Proposal;
  typedef std::shared_ptr<Proposal> ProposalPtr;

  typedef atomic::RelaxedAtomic<int16_t> ProposalCounter;

#define FOR_EACH_PROPOSAL_STATE(V) \
  V(Queued)                        \
  V(Active)                        \
  V(Finished)

  class Proposal{
    //TODO: add TransitionTo
    friend class ServerSession;
   public:
    static const int64_t kDefaultProposalTimeoutSeconds = 120;

    enum class Phase{
      kPhase1,
      kPhase2,
    };

    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_PROPOSAL_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name##State: \
            return stream << #Name;
        FOR_EACH_PROPOSAL_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   private:
    RawProposal raw_;
    uv_loop_t* loop_;
    uv_timer_t timeout_;
    atomic::RelaxedAtomic<State> state_;
    int16_t required_;
    // overall
    ProposalCounter total_votes_; // the total amount of votes received during this proposal
    ProposalCounter total_accepted_; // the total amount of positive votes received during this proposal
    ProposalCounter total_rejected_; // the total amount of negative votes received during this proposal
    ProposalCounter promises_; // the total amount of promises for this proposal
    ProposalCounter accepts_; // the total amount of accepts for this proposal
    ProposalCounter rejects_; // the current amount of negative votes for this phase
    ProposalCounter current_votes_; // the current amount of vote for this phase

    void SetState(const State& state){
      state_ = state;
    }

    static void OnTimeout(uv_timer_t* handle){
      auto proposal = (Proposal*)handle->data;
      DLOG(WARNING) << "proposal " << proposal->GetID() << " has timed out.";
    }

    static int16_t GetNumberOfRequiredVotes();
   public:
    Proposal(const RawProposal& raw, uv_loop_t* loop):
      raw_(raw),
      loop_(loop),
      timeout_(),
      state_(State::kQueuedState),
      required_(GetNumberOfRequiredVotes()),
      total_votes_(0),
      total_accepted_(0),
      total_rejected_(0),
      promises_(0),
      accepts_(0),
      rejects_(0),
      current_votes_(0){
      CHECK_UVRESULT(uv_timer_init(loop, &timeout_), LOG(ERROR), "cannot initialize the proposal timer");
    }
    ~Proposal() = default;

    RawProposal& raw(){
      return raw_;
    }

    RawProposal raw() const{
      return raw_;
    }

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    State GetState() const{
      return (State)state_;
    }

    int16_t GetRequired() const{
      return required_;
    }

    int16_t GetTotalVotes() const{
      return (int16_t)total_votes_;
    }

    int16_t GetTotalAccepted() const{
      return (int16_t)total_accepted_;
    }

    int16_t GetTotalRejected() const{
      return (int16_t)total_rejected_;
    }

    int16_t GetPromises() const{
      return (int16_t)promises_;
    }

    int16_t GetAccepts() const{
      return (int16_t)accepts_;
    }

    int16_t GetRejects() const{
      return (int16_t)rejects_;
    }

    int16_t GetCurrentVotes() const{
      return (int16_t)current_votes_;
    }

    double GetPercentagePromises() const{
      return ((double)GetPromises() / GetCurrentVotes()) * 100.0;
    }

    double GetPercentageAccepts() const{
      return ((double)GetAccepts() / GetCurrentVotes()) * 100.0;
    }

    double GetPercentageRejects() const{
      return ((double)GetRejects() / GetCurrentVotes()) * 100.0;
    }

    double GetTotalPercentageAccepted() const{
      return ((double)GetTotalAccepted() / GetTotalVotes()) * 100.0;
    }

    double GetTotalPercentageRejected() const{
      return ((double)GetTotalRejected() / GetTotalVotes()) * 100.0;
    }

    bool StopTimer(){
      VERIFY_UVRESULT(uv_timer_stop(&timeout_), LOG(ERROR), "cannot stop the proposal timer");
      return true;
    }

    bool StartTimer(){
      VERIFY_UVRESULT(uv_timer_start(&timeout_, &OnTimeout, kDefaultProposalTimeoutSeconds, 0), LOG(ERROR), "cannot start proposal timer");
      return true;
    }

    Timestamp& GetTimestamp() const{
      return raw().timestamp();
    }

    UUID& GetID() const{
      return raw().proposal_id();
    }

    UUID& GetProposer() const{
      return raw().proposer_id();
    }

    BlockHeader& GetValue() const{
      return raw().value();
    }

    bool IsProposedBy(const UUID& id) const{
      return GetProposer() == id;
    }

    void ResetCurrentVotes(){
      rejects_ = 0;
      current_votes_ = 0;
    }

    void CastVote(const UUID& id, const rpc::PromiseMessagePtr& msg){
      DLOG(INFO) << id << " promised proposal";
      total_votes_ += 1;
      total_accepted_ += 1;
      current_votes_ += 1;
      promises_ += 1;
    }

    void CastVote(const UUID& id, const rpc::AcceptedMessagePtr& msg){
      DLOG(INFO) << id << " accepted proposal";
      total_votes_ += 1;
      total_accepted_ += 1;
      current_votes_ += 1;
      accepts_ += 1;
    }

    void CastVote(const UUID& id, const rpc::RejectedMessagePtr& msg){
      DLOG(INFO) << id << " rejected proposal";
      total_votes_ += 1;
      total_rejected_ += 1;
      current_votes_ += 1;
      rejects_ += 1;
    }

    static inline ProposalPtr
    NewInstance(uv_loop_t* loop, const RawProposal& raw){
      return std::make_shared<Proposal>(raw, loop);
    }

    static inline ProposalPtr
    NewInstance(uv_loop_t* loop, const Timestamp& timestamp, const UUID& id, const UUID& proposer, const BlockHeader& value){
      return NewInstance(loop, RawProposal(timestamp, id, proposer, value));
    }
  };

/*  class ProposalJob : public Job{
    //TODO: handle timeouts
   protected:
    ProposalPtr proposal_;

    bool PauseMiner();
    bool ResumeMiner();
    bool ExecutePhase1();
    bool ExecutePhase2();
    bool RejectProposal();
    bool AcceptProposal();
    bool CommitProposal();

    bool SendAcceptedToMiner();
    bool SendRejectedToMiner();
    JobResult DoWork() override;
   public:
    explicit ProposalJob(ProposalPtr proposal):
      Job(nullptr, "ProposalJob"),
      proposal_(std::move(proposal)){}
    ~ProposalJob() override = default;

    ProposalPtr GetProposal() const{
      return proposal_;
    }
  };*/
}

#endif//TOKEN_PROPOSAL_H