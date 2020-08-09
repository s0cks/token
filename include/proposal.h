#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include <set>
#include <condition_variable>
#include "object.h"
#include "block.h"
#include "server.h"

namespace Token{
    class PrepareMessage;
    class Proposal : public Object{
        friend class ProposerThread;
        friend class NodeSession; //TODO: revoke friendship
    public:
        enum Phase{
            kProposalPhase,
            kVotingPhase,
            kCommitPhase,
            kQuorumPhase
        };

        enum class Status{
            kUnknownStatus = 0,
            kAcceptedStatus,
            kRejectedStatus
        };

        static inline uint32_t
        GetNumberOfRequiredResponses(){
            uint32_t peers = Server::GetNumberOfPeers();
            if(peers == 0) return 0;
            else if(peers == 1) return 1;
            return peers / 2;
        }
    private:
        std::recursive_mutex mutex_;
        std::condition_variable_any cond_;
        Phase phase_;
        Status status_;

        std::string proposer_;
        uint32_t height_;
        uint256_t hash_;

        std::set<std::string> accepted_;
        std::set<std::string> rejected_;

        void SetPhase(Phase phase);
        void SetStatus(Status status);

        Proposal(const std::string& proposer, const uint256_t& hash, uint32_t height):
            phase_(kProposalPhase),
            status_(Status::kUnknownStatus),
            proposer_(proposer),
            height_(height),
            hash_(hash),
            accepted_(),
            rejected_(){}

        size_t GetBufferSize() const{ return 0; }
        bool Encode(uint8_t* bytes) const{ return false; }
    public:
        ~Proposal() = default;

        std::string GetProposer() const{
            return proposer_;
        }

        uint32_t GetHeight() const{
            return height_;
        }

        uint256_t GetHash() const{
            return hash_;
        }

        void AcceptProposal(const std::string& node);
        void RejectProposal(const std::string& node);
        void WaitForPhase(Phase phase);
        void WaitForStatus(Status status);
        void WaitForRequiredResponses(uint32_t required=GetNumberOfRequiredResponses());
        bool HasResponseFrom(const std::string& node_id);
        size_t GetNumberOfAccepted();
        size_t GetNumberOfRejected();
        size_t GetNumberOfResponses();
        Phase GetPhase();
        Status GetStatus();
        std::string ToString() const;

        inline bool
        InProposingPhase(){
            return GetPhase() == kProposalPhase;
        }

        inline bool
        InVotingPhase(){
            return GetPhase() == kVotingPhase;
        }

        inline bool
        InCommitPhase(){
            return GetPhase() == kCommitPhase;
        }

        inline bool
        InQuorumPhase(){
            return GetPhase() == kQuorumPhase;
        }

        inline bool
        IsAccepted(){
            return GetStatus() == Status::kAcceptedStatus;
        }

        inline bool
        IsRejected(){
            return GetStatus() == Status::kRejectedStatus;
        }

        static Handle<Proposal> NewInstance(uint32_t height, const uint256_t& hash, const std::string& proposer);

        static Handle<Proposal> NewInstance(Block* block, const std::string& proposer){
            return NewInstance(block->GetHeight(), block->GetSHA256Hash(), proposer);
        }

        static Handle<Proposal> NewInstance(const BlockHeader& block, const std::string& proposer){
            return NewInstance(block.GetHeight(), block.GetHash(), proposer);
        }
    };

    static std::ostream& operator<<(std::ostream& stream, const Proposal::Status& status){
        switch(status){
            case Proposal::Status::kAcceptedStatus:
                stream << "Accepted";
                return stream;
            case Proposal::Status::kRejectedStatus:
                stream << "Rejected";
                return stream;
            case Proposal::Status::kUnknownStatus:
            default:
                stream << "Unknown";
                return stream;
        }
    }

    static std::ostream& operator<<(std::ostream& stream, const Proposal::Phase& phase){
        switch(phase){
            case Proposal::Phase::kProposalPhase:
                stream << "Proposal";
                break;
            case Proposal::Phase::kVotingPhase:
                stream << "Voting";
                break;
            case Proposal::Phase::kCommitPhase:
                stream << "Commit";
                break;
            case Proposal::Phase::kQuorumPhase:
                stream << "Quorum";
                break;
            default:
                stream << "Unknown";
                break;
        }
        return stream;
    }
}

#endif //TOKEN_PROPOSAL_H