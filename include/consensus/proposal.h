#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include <memory>
#include <utility>
#include "block.h"
#include "configuration.h"

#include "job/job.h"

#include "atomic/relaxed_atomic.h"

#include "consensus/quorum.h"
#include "consensus/raw_proposal.h"
#include "consensus/proposal_phase.h"

namespace token{
  class Proposal;
  typedef std::shared_ptr<Proposal> ProposalPtr;

#define FOR_EACH_PROPOSAL_STATUS(V) \
  V(Queued)                         \
  V(Active)                         \
  V(Accepted)                       \
  V(Rejected)

  class Proposal : public Object{
    friend class ServerSession;
   public:
    static const int64_t kDefaultProposalTimeoutSeconds = 120;

    enum Status{
#define DEFINE_STATUS(Name) k##Name##Status,
      FOR_EACH_PROPOSAL_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
        case Status::k##Name##Status: \
            return stream << #Name;
        FOR_EACH_PROPOSAL_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   private:
    // loop
    uv_loop_t* loop_;
    uv_async_t on_prepare_;
    uv_async_t on_promise_;
    uv_async_t on_commit_;
    uv_async_t on_accepted_;
    uv_async_t on_rejected_;
    // phase
    RelaxedAtomic<ProposalPhase> phase_;
    Phase1Quorum p1_quorum_;
    Phase2Quorum p2_quorum_;
    // core
    RawProposal raw_;

    void SetPhase(const ProposalPhase& phase){
      phase_ = phase;
    }

    static void OnPrepare(uv_async_t* handle);
    static void OnPromise(uv_async_t* handle);
    static void OnCommit(uv_async_t* handle);
    static void OnAccepted(uv_async_t* handle);
    static void OnRejected(uv_async_t* handle);
   public:
    Proposal(uv_loop_t* loop,
             const RawProposal& raw,
             const int16_t& required_votes):
      Object(),
      loop_(loop),
      on_prepare_(),
      on_promise_(),
      on_commit_(),
      on_accepted_(),
      on_rejected_(),
      phase_(ProposalPhase::kQueuedPhase),
      p1_quorum_(loop, raw, required_votes),
      p2_quorum_(loop, raw, required_votes),
      raw_(raw){
      on_prepare_.data = this;
      CHECK_UVRESULT(uv_async_init(loop, &on_prepare_, &OnPrepare), LOG(ERROR), "cannot initialize the on_prepare callback");
      on_promise_.data = this;
      CHECK_UVRESULT(uv_async_init(loop, &on_promise_, &OnPromise), LOG(ERROR), "cannot initialize the on_promise callback");
      on_commit_.data = this;
      CHECK_UVRESULT(uv_async_init(loop, &on_commit_, &OnCommit), LOG(ERROR), "cannot initialize the on_commit callback");
      on_accepted_.data = this;
      CHECK_UVRESULT(uv_async_init(loop, &on_accepted_, &OnAccepted), LOG(ERROR), "cannot initialize the on_accepted callback");
      on_rejected_.data = this;
      CHECK_UVRESULT(uv_async_init(loop, &on_rejected_, &OnRejected), LOG(ERROR), "cannot initialize the on_rejected callback");
    }
    ~Proposal() override = default;

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    ProposalPhase GetPhase() const{
      return (ProposalPhase)phase_;
    }

    RawProposal& raw(){
      return raw_;
    }

    RawProposal raw() const{
      return raw_;
    }

    Phase1Quorum& GetPhase1Quorum(){
      return p1_quorum_;
    }

    Phase2Quorum& GetPhase2Quorum(){
      return p2_quorum_;
    }

    bool TransitionToPhase(const ProposalPhase& phase);//TODO: encapsulate

    int64_t GetBufferSize() const{
      return raw().GetBufferSize();
    }

    bool Encode(const BufferPtr& buff) const{
      return raw().Encode(buff);
    }

    bool OnPrepare(){
      VERIFY_UVRESULT(uv_async_send(&on_prepare_), LOG(ERROR), "cannot invoke the on_prepare callback");
      return true;
    }

    bool OnPromise(){
      DLOG(INFO) << "OnPromise";
      VERIFY_UVRESULT(uv_async_send(&on_promise_), LOG(ERROR), "cannot invoke the on_promise callback");
      return true;
    }

    bool OnCommit(){
      VERIFY_UVRESULT(uv_async_send(&on_commit_), LOG(ERROR), "cannot invoke the on_commit callback");
      return true;
    }

    bool OnAccepted(){
      VERIFY_UVRESULT(uv_async_send(&on_accepted_), LOG(ERROR), "cannot invoke the on_accepted callback");
      return true;
    }

    bool OnRejected(){
      VERIFY_UVRESULT(uv_async_send(&on_rejected_), LOG(ERROR), "cannot invoke the on_rejected callback");
      return true;
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "Proposal(";
      ss << ")";
      return ss.str();
    }

#define DEFINE_PHASE_CHECK(Name) \
    inline bool In##Name##Phase() const{ return GetPhase() == ProposalPhase::k##Name##Phase; }
    FOR_EACH_PROPOSAL_PHASE(DEFINE_PHASE_CHECK)
#undef DEFINE_PHASE_CHECK

    static inline ProposalPtr
    NewInstance(uv_loop_t* loop,
                const RawProposal& raw,
                const int16_t& required_votes){
      return std::make_shared<Proposal>(loop, raw, required_votes);
    }

    static inline ProposalPtr
    NewInstance(uv_loop_t* loop, const Timestamp& timestamp, const UUID& id, const UUID& proposer, const BlockHeader& value, const int16_t& required_votes){
      return NewInstance(loop, RawProposal(timestamp, id, proposer, value), required_votes);
    }

    static inline ProposalPtr
    NewInstance(uv_loop_t* loop, const BlockPtr& value, const int16_t& required_votes){
      Timestamp timestamp = Clock::now();
      UUID id = UUID();
      UUID proposer = ConfigurationManager::GetNodeID();
      return NewInstance(loop, timestamp, id, proposer, value->GetHeader(), required_votes);
    }
  };
}

#endif//TOKEN_PROPOSAL_H