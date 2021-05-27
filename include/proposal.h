#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include <memory>
#include <utility>

#include "block.h"
#include "job/job.h"
#include "configuration.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  class Proposal;
  typedef std::shared_ptr<Proposal> ProposalPtr;

  typedef RelaxedAtomic<int16_t> ProposalCounter;

  class RawProposal : public SerializableObject{
   private:
    Timestamp timestamp_;
    UUID id_;
    UUID proposer_;
    BlockHeader value_;
   public:
    RawProposal(const Timestamp& timestamp,
                const UUID& id,
                const UUID& proposer,
                const BlockHeader& value):
       SerializableObject(),
       timestamp_(timestamp),
       id_(id),
       proposer_(proposer),
       value_(value){}
    explicit RawProposal(const BufferPtr& buff):
      SerializableObject(),
      timestamp_(buff->GetTimestamp()),
      id_(buff->GetUUID()),
      proposer_(buff->GetUUID()),
      value_(buff){}
    ~RawProposal() override = default;

    Type GetType() const override{
      return Type::kProposal;
    }

    Timestamp& GetTimestamp(){
      return timestamp_;
    }

    Timestamp GetTimestamp() const{
      return timestamp_;
    }

    UUID& GetID(){
      return id_;
    }

    UUID GetID() const{
      return id_;
    }

    UUID& GetProposer(){
      return proposer_;
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

    bool Write(const BufferPtr& buff) const override{
      SERIALIZE_BASIC_FIELD(timestamp_, Timestamp);
      SERIALIZE_BASIC_FIELD(id_, UUID);
      SERIALIZE_BASIC_FIELD(proposer_, UUID);
      SERIALIZE_FIELD(value, BlockHeader, value_);
      return true;
    }

    bool Write(Json::Writer& writer) const override{
      return writer.StartObject()
          && Json::SetField(writer, "timestamp", timestamp_)
          && Json::SetField(writer, "id", id_)
          && Json::SetField(writer, "proposer", proposer_)
          && writer.Key("value")
          && value_.Write(writer) //TODO: clean
          && writer.EndObject();
    }

    int64_t GetBufferSize() const override{
      return GetSize();
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "Proposal(";
      ss << "timestamp=" << ToUnixTimestamp(timestamp_) << ", ";
      ss << "id=" << id_  << ", ";
      ss << "proposer=" << proposer_ << ", ";
      ss << "value=" << value_;
      ss << ")";
      return ss.str();
    }

    RawProposal& operator=(const RawProposal& proposal) = default;

    friend bool operator==(const RawProposal& a, const RawProposal& b){
      return ToUnixTimestamp(a.timestamp_) == ToUnixTimestamp(b.timestamp_)
          && a.id_ == b.id_
          && a.proposer_ == b.proposer_
          && a.value_ == b.value_;
    }

    friend bool operator!=(const RawProposal& a, const RawProposal& b){
      return !operator==(a, b);
    }

    friend bool operator<(const RawProposal& a, const RawProposal& b){
      return false;
    }

    friend bool operator>(const RawProposal& a, const RawProposal& b){
      return false;
    }

    friend std::ostream& operator<<(std::ostream& stream, const RawProposal& proposal){
      return stream << proposal.ToString();
    }

    static inline int64_t
    GetSize(){
      int64_t size = 0;
      size += sizeof(RawTimestamp);
      size += UUID::GetSize();
      size += UUID::GetSize();
      size += BlockHeader::GetSize();
      return size;
    }
  };

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
    RelaxedAtomic<State> state_;
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
      return raw().GetTimestamp();
    }

    UUID& GetID() const{
      return raw().GetID();
    }

    UUID& GetProposer() const{
      return raw().GetProposer();
    }

    BlockHeader& GetValue() const{
      return raw().GetValue();
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

    void CastVote(const UUID& id, const rpc::AcceptsMessagePtr& msg){
      DLOG(INFO) << id << " accepted proposal";
      total_votes_ += 1;
      total_accepted_ += 1;
      current_votes_ += 1;
      accepts_ += 1;
    }

    void CastVote(const UUID& id, const rpc::RejectsMessagePtr& msg){
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

    static inline ProposalPtr
    NewInstance(uv_loop_t* loop, const BlockPtr& value){
      Timestamp timestamp = Clock::now();
      UUID id = UUID();
      UUID proposer = ConfigurationManager::GetNodeID();
      return NewInstance(loop, timestamp, id, proposer, value->GetHeader());
    }
  };

  class ProposalJob : public Job{
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
  };
}

#endif//TOKEN_PROPOSAL_H