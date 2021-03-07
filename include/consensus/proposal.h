#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include "block.h"
#include "block_header.h"
#include "configuration.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  class RawProposal{
   private:
    BlockHeader block_;
    UUID proposer_;
   public:
    RawProposal(const BlockHeader& blk, const UUID& proposer):
      block_(blk),
      proposer_(proposer){}
    RawProposal(const BufferPtr& buff):
      block_(buff),
      proposer_(buff->GetUUID()){}
    RawProposal(const RawProposal& proposal):
      block_(proposal.block_),
      proposer_(proposal.proposer_){}
    ~RawProposal() = default;

    BlockHeader& GetBlock(){
      return block_;
    }

    BlockHeader GetBlock() const{
      return block_;
    }

    int64_t GetHeight() const{
      return block_.GetHeight();
    }

    Timestamp GetTimestamp() const{
      return block_.GetTimestamp();
    }

    Hash GetPreviousHash() const{
      return block_.GetPreviousHash();
    }

    Hash GetHash() const{
      return block_.GetHash();
    }

    UUID GetProposer() const{
      return proposer_;
    }

    bool Encode(const BufferPtr& buff) const{
      return block_.Write(buff)
          && buff->PutUUID(proposer_);
    }

    void operator=(const RawProposal& proposal){
      block_ = proposal.block_;
      proposer_ = proposal.proposer_;
    }

    friend bool operator==(const RawProposal& a, const RawProposal& b){
      return a.block_ == b.block_ && a.proposer_ == b.proposer_;
    }

    friend bool operator!=(const RawProposal& a, const RawProposal& b){
      return !operator==(a, b);
    }

    friend std::ostream& operator<<(std::ostream& stream, const RawProposal& proposal){
      stream << "Proposal(";
      stream << FormatTimestampReadable(proposal.GetTimestamp()) << ", ";
      stream << "#" << proposal.GetHeight() << ", ";
      stream << proposal.GetHash() << ", ";
      stream << proposal.GetProposer();
      stream << ")";
      return stream;
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
    V(Proposal)                    \
    V(Voting)                      \
    V(Commit)                      \
    V(Quorum)

#define FOR_EACH_PROPOSAL_RESULT(V) \
    V(None)                         \
    V(Accepted)                     \
    V(Rejected)


#define FOR_EACH_PROPOSAL_CALLBACK(V) \
  V(Prepare, on_prepare)   \
  V(Promise, on_promise)   \
  V(Commit, on_commit)     \
  V(Accepted, on_accepted) \
  V(Rejected, on_rejected)

  class Proposal;
  typedef std::shared_ptr<Proposal> ProposalPtr;

  class PeerSession;
  class PrepareMessage;
  class Proposal : public Object{
    friend class ServerSession;
    friend class ProposalHandler;
    friend class BlockDiscoveryThread;
   public:
    static const int64_t kDefaultProposalTimeoutSeconds = 120;

    enum Phase{
#define DEFINE_PHASE(Name) k##Name##Phase,
      FOR_EACH_PROPOSAL_PHASE(DEFINE_PHASE)
#undef DEFINE_PHASE
    };

    friend std::ostream& operator<<(std::ostream& stream, const Phase& phase){
      switch(phase){
#define DEFINE_TOSTRING(Name) \
        case Phase::k##Name##Phase: \
          return stream << #Name;
        FOR_EACH_PROPOSAL_PHASE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    enum Result{
#define DEFINE_RESULT(Name) k##Name,
      FOR_EACH_PROPOSAL_RESULT(DEFINE_RESULT)
#undef DEFINE_RESULT
    };

    friend std::ostream& operator<<(std::ostream& stream, const Result& result){
      switch(result){
#define DEFINE_TOSTRING(Name) \
        case Result::k##Name: \
            return stream << #Name;
        FOR_EACH_PROPOSAL_RESULT(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   private:
    RawProposal raw_;
    RelaxedAtomic<Phase> phase_;
    RelaxedAtomic<Result> result_;
    RelaxedAtomic<int64_t> accepted_;
    RelaxedAtomic<int64_t> rejected_;
    uv_loop_t* loop_;
    uv_timer_t timeout_;

#define DEFINE_CALLBACK_HANDLE(Name, Callback) \
    uv_async_t Callback##_;
    FOR_EACH_PROPOSAL_CALLBACK(DEFINE_CALLBACK_HANDLE)
#undef DEFINE_CALLBACK_HANDLE

    void SetPhase(const Phase& phase){
      phase_ = phase;
    }

    void SetResult(const Result& result){
      result_ = result;
    }

    static int GetRequiredNumberOfPeers();
    static void OnTimeout(uv_timer_t* handle);

#define DECLARE_CALLBACK_HANDLER(Name, Callback) \
    static void On##Name(uv_async_t* handle);
    FOR_EACH_PROPOSAL_CALLBACK(DECLARE_CALLBACK_HANDLER)
#undef DECLARE_CALLBACK_HANDLER

#define DEFINE_CALLBACK_SEND(Name, Callback) \
    bool Send##Name(){                       \
        int err;                             \
        if((err = uv_async_send(&Callback##_)) != 0){ \
            LOG(ERROR) << "cannot send " << #Name << " callback: " << uv_strerror(err); \
            return false;                    \
        }                                    \
        return true;                         \
    }
    FOR_EACH_PROPOSAL_CALLBACK(DEFINE_CALLBACK_SEND)
#undef DEFINE_CALLBACK_SEND

    bool StartProposalTimeout(){
      int err;
      if((err = uv_timer_start(&timeout_, &OnTimeout, kDefaultProposalTimeoutSeconds, 0)) != 0){
        LOG(ERROR) << "cannot start proposal timeout: " << uv_strerror(err);
        return false;
      }
      return true;
    }

    bool StopProposalTimeout(){
      int err;
      if((err = uv_timer_stop(&timeout_)) != 0){
        LOG(ERROR) << "cannot stop proposal timeout: " << uv_strerror(err);
        return false;
      }
      return true;
    }
   public:
    Proposal(uv_loop_t* loop, const RawProposal& proposal):
      Object(),
      raw_(proposal),
      phase_(Proposal::kProposalPhase),
      result_(Proposal::kNone),
      accepted_(0),
      rejected_(0),
      loop_(loop),
      timeout_(),
      on_prepare_(),
      on_promise_(),
      on_accepted_(),
      on_rejected_(){

      int err;
      if((err = uv_timer_init(loop, &timeout_)) != 0){
        LOG(ERROR) << "cannot register proposal timeout timer: " << uv_strerror(err);
        return;
      }

#define INIT_CALLBACK(Name, Callback) \
      Callback##_.data = this;        \
      if((err = uv_async_init(loop_, &Callback##_, &On##Name)) != 0){ \
        LOG(ERROR) << "cannot register " << #Name << " callback: " << uv_strerror(err); \
        return;                       \
      }
      FOR_EACH_PROPOSAL_CALLBACK(INIT_CALLBACK)
#undef INIT_CALLBACK
    }
    Proposal(uv_loop_t* loop, const UUID& proposer, const BlockHeader& blk):
      Proposal(loop, RawProposal(blk, proposer)){}
    Proposal(uv_loop_t* loop, const UUID& proposer, const BlockPtr& blk):
      Proposal(loop, proposer, blk->GetHeader()){}
    ~Proposal() = default;

    Phase GetPhase() const{
      return phase_;
    }

    Result GetResult() const{
      return result_;
    }

    RawProposal& raw(){
      return raw_;
    }

    RawProposal raw() const{
      return raw_;
    }

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    int64_t GetNumberOfAcceptedVotes() const{
      return accepted_;
    }

    int64_t GetNumberOfRejectedVotes() const{
      return rejected_;
    }

    int64_t GetNumberOfVotes() const{
      return accepted_ + rejected_;
    }

    bool TransitionToPhase(const Phase& phase);//TODO: encapsulate
    void AcceptProposal(const UUID& node);
    void RejectProposal(const UUID& node);
    void WaitForRequiredResponses(int required = GetRequiredNumberOfPeers());

    int64_t GetBufferSize() const{
      return RawProposal::GetSize();
    }

    bool Encode(const BufferPtr& buff) const{
      return raw_.Encode(buff);
    }

    bool Equals(const ProposalPtr& proposal) const{
      return raw_ == proposal->raw_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "Proposal(";
      ss << ")";
      return ss.str();
    }

#define DEFINE_PHASE_CHECK(Name) \
    inline bool Is##Name() { return GetPhase() == Proposal::k##Name##Phase; }
    FOR_EACH_PROPOSAL_PHASE(DEFINE_PHASE_CHECK)
#undef DEFINE_PHASE_CHECK

#define DEFINE_RESULT_CHECK(Name) \
    inline bool Is##Name(){ return GetResult() == Proposal::k##Name; }
    FOR_EACH_PROPOSAL_RESULT(DEFINE_RESULT_CHECK)
#undef DEFINE_RESULT_CHECK

    static inline ProposalPtr
    NewInstance(uv_loop_t* loop, const UUID& proposer, const BlockPtr& blk){
      return std::make_shared<Proposal>(loop, proposer, blk);
    }

    static inline ProposalPtr
    NewInstance(uv_loop_t* loop, const BlockPtr& blk){
      return NewInstance(loop, ConfigurationManager::GetID(TOKEN_CONFIGURATION_NODE_ID), blk);
    }
  };
#undef FOR_EACH_PROPOSAL_CALLBACK
}

#endif //TOKEN_PROPOSAL_H