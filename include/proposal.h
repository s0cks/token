#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include "block.h"
#include "block_header.h"
#include "configuration.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  class RpcSession;

  class RawProposal{
   private:
    Timestamp timestamp_;
    UUID proposal_;
    UUID proposer_;
    BlockHeader value_;
   public:
    RawProposal(const Timestamp& timestamp, const UUID& uuid, const UUID& proposer, const BlockHeader& blk):
      timestamp_(timestamp),
      proposal_(uuid),
      proposer_(proposer),
      value_(blk){}
    RawProposal(const BlockHeader& blk):
      RawProposal(Clock::now(), UUID(), ConfigurationManager::GetID(TOKEN_CONFIGURATION_NODE_ID), blk){}
    RawProposal(const BufferPtr& buff):
      timestamp_(buff->GetTimestamp()),
      proposal_(buff->GetUUID()),
      proposer_(buff->GetUUID()),
      value_(buff){}
    RawProposal(const RawProposal& proposal):
      timestamp_(proposal.timestamp_),
      proposal_(proposal.proposal_),
      proposer_(proposal.proposer_),
      value_(proposal.value_){}
    ~RawProposal() = default;

    Timestamp GetTimestamp() const{
      return timestamp_;
    }

    UUID GetUUID() const{
      return proposal_;
    }

    UUID GetProposer() const{
      return proposer_;
    }

    BlockHeader& GetValue(){
      return value_;
    }

    BlockHeader GetValue() const{
      return value_;
    }

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += sizeof(uint64_t); // timestamp
      size += UUID::GetSize(); // proposal
      size += UUID::GetSize();
      size += BlockHeader::GetSize();
      return size;
    }

    bool Encode(const BufferPtr& buff) const{
      SERIALIZE_BASIC_FIELD(timestamp_, Timestamp);
      SERIALIZE_BASIC_FIELD(proposal_, UUID);
      SERIALIZE_BASIC_FIELD(proposer_, UUID);
      SERIALIZE_FIELD(value, BlockHeader, &value_);
      return true;
    }

    void operator=(const RawProposal& proposal){
      timestamp_ = proposal.timestamp_;
      proposal_ = proposal.proposal_;
      proposer_ = proposal.proposer_;
      value_ = proposal.value_;
    }

    friend bool operator<(const RawProposal& a, const RawProposal& b){
      return a.value_ < b.value_;
    }

    friend bool operator>(const RawProposal& a, const RawProposal& b){
      return a.value_ > b.value_;
    }

    friend bool operator==(const RawProposal& a, const RawProposal& b){
      return a.timestamp_ == b.timestamp_
          && a.proposal_ == b.proposal_
          && a.proposer_ == b.proposer_
          && a.value_ == b.value_;
    }

    friend bool operator!=(const RawProposal& a, const RawProposal& b){
      return !operator==(a, b);
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "Proposal(";
        ss << "timestamp=" << FormatTimestampReadable(GetTimestamp()) << ", ";
        ss << "proposal=" << GetUUID() << ", ";
        ss << "proposer=" << GetProposer() << ", ";
        ss << "value=" << GetValue().GetHash();
      ss << ")";
      return ss.str();
    }

    friend std::ostream& operator<<(std::ostream& stream, const RawProposal& proposal){
      return stream << proposal.GetUUID();
    }

    static inline int64_t
    GetSize(){
      int64_t size = 0;
      size += BlockHeader::GetSize();
      size += UUID::GetSize();
      return size;
    }
  };

#define FOR_EACH_PROPOSAL_PHASE(V) \
  V(Queued)                        \
  V(Prepare)                      \
  V(Commit)

#define FOR_EACH_PROPOSAL_STATUS(V) \
  V(Queued)                         \
  V(Active)                         \
  V(Accepted)                       \
  V(Rejected)

  class Proposal;
  typedef std::shared_ptr<Proposal> ProposalPtr;

  enum ProposalPhase{
#define DEFINE_PHASE(Name) k##Name##Phase,
    FOR_EACH_PROPOSAL_PHASE(DEFINE_PHASE)
#undef DEFINE_PHASE
  };

  static std::ostream& operator<<(std::ostream& stream, const ProposalPhase& phase){
    switch(phase){
#define DEFINE_TOSTRING(Name) \
      case ProposalPhase::k##Name##Phase: \
        return stream << #Name;
      FOR_EACH_PROPOSAL_PHASE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
      default:
        return stream << "Unknown";
    }
  }

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
#ifdef TOKEN_DEBUG
      LOG(INFO) << "waiting for " << GetRequiredVotes() << " votes....";
#endif//TOKEN_DEBUG
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
#ifdef TOKEN_DEBUG
      LOG(INFO) << uuid << " promised proposal: " << GetProposal();
#endif//TOKEN_DEBUG
      promised_ += 1;
    }

    void RejectProposal(const UUID& uuid){
#ifdef TOKEN_DEBUG
      LOG(INFO) << uuid << " rejected proposal: " << GetProposal();
#endif//TOKEN_DEBUG
      rejected_ += 1;
    }

    static void OnTimeout(uv_timer_t* handle){
      LOG(ERROR) << "proposal phase 1 timeout.";
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
#ifdef TOKEN_DEBUG
      LOG(INFO) << "stopping phase 1 timer....";
#endif//TOKEN_DEBUG
      VERIFY_UVRESULT(uv_timer_start(&timeout_, &OnTimeout, kPhase1TimeoutMilliseconds, 0), LOG(ERROR), "cannot start phase 1 quorum timer");
      return true;
    }

    bool StopTimer(){
#ifdef TOKEN_DEBUG
      LOG(INFO) << "stopping phase 1 timer....";
#endif//TOKEN_DEBUG
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
#ifdef TOKEN_DEBUG
      LOG(INFO) << uuid.ToStringAbbreviated() << " accepted proposal: " << GetProposal();
#endif//TOKEN_DEBUG
      accepted_ += 1;
    }

    void RejectProposal(const UUID& uuid){
#ifdef TOKEN_DEBUG
      LOG(INFO) << uuid << " rejected proposal: " << GetProposal();
#endif//TOKEN_DEBUG
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
#ifdef TOKEN_DEBUG
      LOG(INFO) << "starting phase 2 timer....";
#endif//TOKEN_DEBUG
      VERIFY_UVRESULT(uv_timer_start(&timeout_, &OnTimeout, kPhase2TimeoutMilliseconds, 0), LOG(ERROR), "cannot start phase 2 timer");
      return true;
    }

    bool StopTimer(){
#ifdef TOKEN_DEBUG
      LOG(INFO) << "stopping phase 1 timer....";
#endif//TOKEN_DEBUG
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

  class PeerSession;
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
    uv_loop_t* loop_;
    RpcSession* session_;
    RawProposal raw_;
    RelaxedAtomic<ProposalPhase> phase_;
    Phase1Quorum p1_quorum_;
    Phase2Quorum p2_quorum_;

    uv_async_t on_prepare_;
    uv_async_t on_promise_;
    uv_async_t on_commit_;
    uv_async_t on_accepted_;
    uv_async_t on_rejected_;

    void SetPhase(const ProposalPhase& phase){
      phase_ = phase;
    }

    static void OnPrepare(uv_async_t* handle);
    static void OnPromise(uv_async_t* handle);
    static void OnCommit(uv_async_t* handle);
    static void OnAccepted(uv_async_t* handle);
    static void OnRejected(uv_async_t* handle);
   public:
    Proposal(RpcSession* session,
             uv_loop_t* loop,
             const RawProposal& proposal,
             const int16_t& required_votes):
      Object(),
      loop_(loop),
      session_(session),
      raw_(proposal),
      phase_(ProposalPhase::kQueuedPhase),
      p1_quorum_(loop, proposal, required_votes),
      p2_quorum_(loop, proposal, required_votes),
      on_prepare_(),
      on_promise_(),
      on_commit_(),
      on_accepted_(),
      on_rejected_(){
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
    ~Proposal() = default;

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    RpcSession* GetSession() const{
      return session_;
    }

    ProposalPhase GetPhase() const{
      return phase_;
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
      return RawProposal::GetSize();
    }

    bool Encode(const BufferPtr& buff) const{
      return raw_.Encode(buff);
    }

    bool Equals(const ProposalPtr& proposal) const{
      return raw_ == proposal->raw_;
    }

    bool OnPrepare(){
      VERIFY_UVRESULT(uv_async_send(&on_prepare_), LOG(ERROR), "cannot invoke the on_prepare callback");
      return true;
    }

    bool OnPromise(){
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

    std::string ToString() const{
      std::stringstream ss;
      ss << "Proposal(";
      ss << ")";
      return ss.str();
    }

#define DEFINE_PHASE_CHECK(Name) \
    inline bool In##Name##Phase() const{ return phase_ == ProposalPhase::k##Name##Phase; }
    FOR_EACH_PROPOSAL_PHASE(DEFINE_PHASE_CHECK)
#undef DEFINE_PHASE_CHECK

    static inline ProposalPtr
    NewInstance(RpcSession* session, uv_loop_t* loop, const RawProposal& proposal, const int16_t& required_votes){
      return std::make_shared<Proposal>(session, loop, proposal, required_votes);
    }

    static inline ProposalPtr
    NewInstance(RpcSession* session, uv_loop_t* loop, const BlockPtr& blk, const int16_t& required_votes){
      return std::make_shared<Proposal>(session, loop, RawProposal(blk->GetHeader()), required_votes);
    }
  };
#undef FOR_EACH_PROPOSAL_CALLBACK
}

#endif //TOKEN_PROPOSAL_H