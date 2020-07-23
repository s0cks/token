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
    public:
        enum Phase{
            kProposalPhase,
            kVotingPhase,
            kCommitPhase,
            kQuorumPhase
        };

        static inline uint32_t
        GetRequiredNumberOfVotes(){
            uint32_t peers = Server::GetNumberOfPeers();
            if(peers == 0) return 0;
            else if(peers == 1) return 1;
            return peers / 2;
        }

        static inline uint32_t
        GetRequiredNumberOfCommits(){
            uint32_t peers = Server::GetNumberOfPeers();
            if(peers == 0) return 0;
            else if(peers == 1) return 1;
            return peers / 2;
        }
    private:
        std::recursive_mutex mutex_;
        std::condition_variable_any cond_;
        Phase phase_;
        std::string proposer_;
        uint32_t height_;
        uint256_t hash_;
        std::set<std::string> votes_;
        std::set<std::string> commits_;

        void WaitForPhase(Phase phase);

        Proposal(const std::string& proposer, const uint256_t& hash, uint32_t height):
            phase_(kProposalPhase),
            proposer_(proposer),
            height_(height),
            votes_(),
            hash_(hash),
            commits_(){}
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

        void SetPhase(Phase phase);
        void Vote(const std::string& node_id);
        void Commit(const std::string& node_id);
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

        static Handle<Proposal> NewInstance(uint32_t height, const uint256_t& hash, const std::string& proposer);

        static Handle<Proposal> NewInstance(Block* block, const std::string& proposer){
            return NewInstance(block->GetHeight(), block->GetSHA256Hash(), proposer);
        }

        static Handle<Proposal> NewInstance(const BlockHeader& block, const std::string& proposer){
            return NewInstance(block.GetHeight(), block.GetHash(), proposer);
        }
    };
}

#endif //TOKEN_PROPOSAL_H