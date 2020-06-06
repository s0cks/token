#ifndef TOKEN_PAXOS_H
#define TOKEN_PAXOS_H

#include "common.h"
#include "block_chain.h"

namespace Token{
    class PrepareMessage;
    class Proposal{
    private:
        std::string proposer_;
        uint32_t height_;
        uint256_t hash_;

        pthread_mutex_t votes_mutex_;
        std::set<std::string> votes_;

        pthread_mutex_t commits_mutex_;
        std::set<std::string> commits_;
    public:
        Proposal():
            proposer_(),
            height_(0),
            hash_(),
            votes_(),
            votes_mutex_(),
            commits_(),
            commits_mutex_(){}
        Proposal(const std::string& node, uint32_t height, const uint256_t& hash):
            proposer_(node),
            height_(height),
            hash_(hash),
            votes_(),
            votes_mutex_(),
            commits_(),
            commits_mutex_(){}
        Proposal(const std::string& node, Block* block): Proposal(node, block->GetHeight(), block->GetHash()){}
        ~Proposal(){}

        std::string GetProposer() const{
            return proposer_;
        }

        uint32_t GetHeight() const{
            return height_;
        }

        uint256_t GetHash() const{
            return hash_;
        }

        bool Vote(const std::string& node_id){
            pthread_mutex_trylock(&votes_mutex_);
            bool voted = votes_.insert(node_id).second;
            pthread_mutex_unlock(&votes_mutex_);
            return voted;
        }

        bool HasVote(const std::string& node_id){
            pthread_mutex_trylock(&votes_mutex_);
            bool found = votes_.find(node_id) != votes_.end();
            pthread_mutex_unlock(&votes_mutex_);
            return found;
        }

        bool IsValid() const{
            return height_ == (BlockChain::GetHeight() + 1);
        }

        bool Commit(const std::string& node_id){
            pthread_mutex_trylock(&commits_mutex_);
            bool committed = commits_.insert(node_id).second;
            pthread_mutex_unlock(&commits_mutex_);
            return committed;
        }

        uint32_t GetNumberOfVotes(){
            pthread_mutex_trylock(&votes_mutex_);
            uint32_t votes = votes_.size();
            pthread_mutex_unlock(&votes_mutex_);
            return votes;
        }

        uint32_t GetNumberOfCommits(){
            pthread_mutex_trylock(&commits_mutex_);
            uint32_t commits = commits_.size();
            pthread_mutex_unlock(&commits_mutex_);
            return commits;
        }

        friend bool operator==(const Proposal& a, const Proposal& b){
            return a.proposer_ == b.proposer_ &&
                    a.height_ == b.height_ &&
                    a.hash_ == b.hash_;
        }

        friend bool operator!=(const Proposal& a, const Proposal& b){
            return !operator==(a, b);
        }

        friend bool operator<(const Proposal& a, const Proposal& b){
            return a.GetHeight() < b.GetHeight();
        }

        friend std::ostream& operator<<(std::ostream& stream, const Proposal& proposal){
            stream << "#" << proposal.GetHeight() << "[" << proposal.GetHash() << "]";
            return stream;
        }
    };
}

#endif //TOKEN_PAXOS_H