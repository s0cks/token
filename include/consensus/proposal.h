#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include <mutex>
#include <condition_variable>
#include "block.h"

namespace Token{
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

  class Proposal;
  typedef std::shared_ptr<Proposal> ProposalPtr;

  class PeerSession;
  class PrepareMessage;
  class Proposal : public Object{
    friend class ServerSession;
    friend class ProposalHandler;
    friend class BlockDiscoveryThread;
   public:
    enum Phase{
#define DEFINE_PHASE(Name) k##Name##Phase,
      FOR_EACH_PROPOSAL_PHASE(DEFINE_PHASE)
#undef DEFINE_PHASE
    };

    friend std::ostream& operator<<(std::ostream& stream, const Phase& phase){
      switch(phase){
#define DEFINE_TOSTRING(Name) \
                case Phase::k##Name##Phase: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_PROPOSAL_PHASE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
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
                    stream << #Name; \
                    return stream;
        FOR_EACH_PROPOSAL_RESULT(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }
   private:
    std::mutex mutex_;
    std::condition_variable cond_;
    Phase phase_;
    Result result_;
    RawProposal raw_;
    std::set<std::string> accepted_;
    std::set<std::string> rejected_;

    void SetPhase(const Phase& phase);
    void SetResult(const Result& result);

    static int GetRequiredNumberOfPeers();
   public:
    Proposal(const RawProposal& proposal):
      Object(),
      phase_(Proposal::kProposalPhase),
      result_(Proposal::kNone),
      raw_(proposal),
      accepted_(),
      rejected_(){}
    Proposal(const UUID& proposer, const BlockHeader& blk):
      Object(),
      phase_(Proposal::kProposalPhase),
      result_(Proposal::kNone),
      raw_(blk, proposer),
      accepted_(),
      rejected_(){}
    Proposal(BlockPtr blk, const UUID& proposer):
      Object(),
      phase_(Proposal::kProposalPhase),
      result_(Proposal::kNone),
      raw_(blk->GetHeader(), proposer),
      accepted_(),
      rejected_(){}
    Proposal(const BufferPtr& buff):
      Object(),
      phase_(Proposal::kProposalPhase),
      result_(Proposal::kNone),
      raw_(buff),
      accepted_(),
      rejected_(){}
    ~Proposal() = default;

    RawProposal& GetRaw(){
      return raw_;
    }

    BlockHeader& GetBlock(){
      return raw_.GetBlock();
    }

    BlockHeader GetBlock() const{
      return raw_.GetBlock();
    }

    Timestamp GetTimestamp() const{
      return raw_.GetTimestamp();
    }

    int64_t GetHeight() const{
      return raw_.GetHeight();
    }

    Hash GetHash() const{
      return raw_.GetHash();
    }

    UUID GetProposer() const{
      return raw_.GetProposer();
    }

    int64_t GetBufferSize() const{
      return RawProposal::GetSize();
    }

    bool Encode(const BufferPtr& buff) const{
      return raw_.Encode(buff);
    }

    bool Equals(const ProposalPtr& proposal) const{
      return raw_ == proposal->raw_;
    }

    bool IsStartedBy(const UUID& uuid){
      return GetProposer() == uuid;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "Proposal(#" << GetHeight() << ", " << GetHash() << ", " << GetProposer() << ")";
      return ss.str();
    }

    #ifdef TOKEN_ENABLE_SERVER
    PeerSession* GetPeer() const;
    #endif//TOKEN_ENABLE_SERVER

    Phase GetPhase();
    Result GetResult();
    int64_t GetNumberOfAccepted();
    int64_t GetNumberOfRejected();
    int64_t GetNumberOfResponses();
    bool Commit();
    bool TransitionToPhase(const Phase& phase);
    bool HasResponseFrom(const std::string& node_id);
    void AcceptProposal(const std::string& node);
    void RejectProposal(const std::string& node);
    void WaitForPhase(const Phase& phase);
    void WaitForResult(const Result& result);
    void WaitForRequiredResponses(int required = GetRequiredNumberOfPeers());

#define DEFINE_PHASE_CHECK(Name) \
        inline bool Is##Name() { return GetPhase() == Proposal::k##Name##Phase; }
    FOR_EACH_PROPOSAL_PHASE(DEFINE_PHASE_CHECK)
#undef DEFINE_PHASE_CHECK

#define DEFINE_RESULT_CHECK(Name) \
        inline bool Is##Name(){ return GetResult() == Proposal::k##Name; }
    FOR_EACH_PROPOSAL_RESULT(DEFINE_RESULT_CHECK)
#undef DEFINE_RESULT_CHECK
  };
}

#endif //TOKEN_PROPOSAL_H