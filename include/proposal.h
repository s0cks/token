#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include <set>
#include <condition_variable>
#include "uuid.h"
#include "block.h"

namespace Token{
    class RawProposal{
        //TODO:
        // - Merge RawProposal w/ BlockHeader?
    public:
        static const int64_t kSize = sizeof(int64_t)
                                   + sizeof(int64_t)
                                   + Hash::kSize
                                   + UUID::kSize;

        struct TimestampComparator{
            bool operator()(const RawProposal& a, const RawProposal& b){
                return a.GetTimestamp() < b.GetTimestamp();
            }
        };

        struct HeightComparator{
            bool operator()(const RawProposal& a, const RawProposal& b){
                return a.GetHeight() < b.GetHeight();
            }
        };
    private:
        int64_t timestamp_;
        int64_t height_;
        Hash hash_;
        UUID proposer_;
    public:
        RawProposal(const UUID& proposer, int64_t height, const Hash& hash, int64_t timestamp=GetCurrentTimestamp()):
            timestamp_(timestamp),
            height_(height),
            hash_(hash),
            proposer_(proposer){}
        RawProposal(const UUID& proposer, const BlockHeader& blk, int64_t timestamp=GetCurrentTimestamp()):
            RawProposal(proposer, blk.GetHeight(), blk.GetHash(), timestamp){}
        RawProposal(Buffer* buff):
            timestamp_(buff->GetLong()),
            height_(buff->GetLong()),
            hash_(buff->GetHash()),
            proposer_(buff){}
        RawProposal(const RawProposal& proposal):
            timestamp_(proposal.GetTimestamp()),
            height_(proposal.GetHeight()),
            hash_(proposal.GetHash()),
            proposer_(proposal.GetProposer()){}
        ~RawProposal() = default;

        int64_t GetTimestamp() const{
            return timestamp_;
        }

        int64_t GetHeight() const{
            return height_;
        }

        Hash GetHash() const{
            return hash_;
        }

        UUID GetProposer() const{
            return proposer_;
        }

        bool Encode(Buffer* buff) const{
            buff->PutLong(GetTimestamp());
            buff->PutLong(GetHeight());
            buff->PutHash(GetHash());
            proposer_.Write(buff);
            return true;
        }

        void operator=(const RawProposal& proposal){
            timestamp_ = proposal.GetTimestamp();
            height_ = proposal.GetHeight();
            hash_ = proposal.GetHash();
            proposer_ = proposal.GetProposer();
        }

        friend bool operator==(const RawProposal& a, const RawProposal& b){
            return a.timestamp_ == b.timestamp_
                && a.height_ == b.height_
                && a.hash_ == b.hash_
                && a.proposer_ == b.proposer_;
        }

        friend bool operator!=(const RawProposal& a, const RawProposal& b){
            return !operator==(a, b);
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
                default:
                    stream << "Unknown";
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
                default:
                    stream << "Unknown";
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
            Object(Type::kProposalType),
            phase_(Proposal::kProposalPhase),
            result_(Proposal::kNone),
            raw_(proposal),
            accepted_(),
            rejected_(){}
        Proposal(const UUID& proposer, int64_t height, const Hash& hash, int64_t timestamp=GetCurrentTimestamp()):
            Object(Type::kProposalType),
            phase_(Proposal::kProposalPhase),
            result_(Proposal::kNone),
            raw_(proposer, height, hash, timestamp),
            accepted_(),
            rejected_(){}
        Proposal(Block* blk, const UUID& proposer, int64_t timestamp=GetCurrentTimestamp()):
            Proposal(proposer, blk->GetHeight(), blk->GetHash(), timestamp){}
        Proposal(const BlockHeader& blk, const UUID& proposer, int64_t timestamp=GetCurrentTimestamp()):
            Proposal(proposer, blk.GetHeight(), blk.GetHash(), timestamp){}
        Proposal(Buffer* buff):
            Object(Type::kProposalType),
            phase_(Proposal::kProposalPhase),
            result_(Proposal::kNone),
            raw_(buff),
            accepted_(),
            rejected_(){}
        ~Proposal() = default;

        RawProposal& GetRaw(){
            return raw_;
        }

        int64_t GetTimestamp() const{
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
            return RawProposal::kSize;
        }

        bool Encode(Buffer* buff) const{
            return raw_.Encode(buff);
        }

        std::shared_ptr<PeerSession> GetPeer() const;
        Phase GetPhase();
        Result GetResult();
        int64_t GetNumberOfAccepted();
        int64_t GetNumberOfRejected();
        int64_t GetNumberOfResponses();
        bool HasResponseFrom(const std::string& node_id);
        void AcceptProposal(const std::string& node);
        void RejectProposal(const std::string& node);
        void WaitForPhase(const Phase& phase);
        void WaitForResult(const Result& result);
        void WaitForRequiredResponses(int required=GetRequiredNumberOfPeers());

#define DEFINE_PHASE_CHECK(Name) \
        inline bool Is##Name() { return GetPhase() == Proposal::k##Name##Phase; }
        FOR_EACH_PROPOSAL_PHASE(DEFINE_PHASE_CHECK)
#undef DEFINE_PHASE_CHECK

#define DEFINE_RESULT_CHECK(Name) \
        inline bool Is##Name(){ return GetResult() == Proposal::k##Name; }
        FOR_EACH_PROPOSAL_RESULT(DEFINE_RESULT_CHECK)
#undef DEFINE_RESULT_CHECK
    };

    typedef std::shared_ptr<Proposal> ProposalPtr;
}

#endif //TOKEN_PROPOSAL_H