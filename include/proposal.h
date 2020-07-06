#ifndef TOKEN_PROPOSAL_H
#define TOKEN_PROPOSAL_H

#include <set>
#include <condition_variable>
#include "object.h"
#include "block.h"
#include "node/node.h"

namespace Token{
    class PrepareMessage;
    class Proposal : public Object{
    public:
        enum Phase{
            kProposalPhase,
            kVotingPhase,
            kCommitPhase,
            kQuorumPhase
        };

        static inline uint32_t
        GetRequiredNumberOfVotes(){
            uint32_t peers = Node::GetNumberOfPeers();
            if(peers == 0) return 0;
            else if(peers == 1) return 1;
            return peers / 2;
        }

        static inline uint32_t
        GetRequiredNumberOfCommits(){
            uint32_t peers = Node::GetNumberOfPeers();
            if(peers == 0) return 0;
            else if(peers == 1) return 1;
            return peers / 2;
        }
    private:
        std::recursive_mutex mutex_;
        std::condition_variable_any cond_;
        Phase phase_;
        NodeInfo proposer_;
        uint256_t hash_;
        std::set<NodeInfo> votes_;
        std::set<NodeInfo> commits_;

        void WaitForPhase(Phase phase);

        Proposal(const NodeInfo& proposer, const uint256_t& hash):
            phase_(kProposalPhase),
            proposer_(proposer),
            hash_(hash),
            votes_(),
            commits_(){}
    public:
        ~Proposal() = default;

        NodeInfo GetProposer() const{
            return proposer_;
        }

        uint256_t GetHash() const{
            return hash_;
        }

        void SetPhase(Phase phase);
        void Vote(const NodeInfo& info);
        void Commit(const NodeInfo& info);
        void WaitForRequiredVotes(uint32_t required=GetRequiredNumberOfVotes());
        void WaitForRequiredCommits(uint32_t required=GetRequiredNumberOfCommits());
        uint32_t GetNumberOfVotes();
        uint32_t GetNumberOfCommits();
        Phase GetPhase();
        std::string ToString() const;

        inline void
        WaitForQuorum(){
            WaitForPhase(kQuorumPhase);
        }

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

        static void SetCurrentProposal(Proposal* proposal);
        static bool HasCurrentProposal();
        static Proposal* GetCurrentProposal();
        static Proposal* NewInstance(const uint256_t& hash, const NodeInfo& proposer);

        static Proposal* NewInstance(Block* block, const NodeInfo& proposer){
            return NewInstance(block->GetHash(), proposer);
        }

        static Proposal* NewInstance(const BlockHeader& block, const NodeInfo& proposer){
            return NewInstance(block.GetHash(), proposer);
        }
    };
}

#endif //TOKEN_PROPOSAL_H