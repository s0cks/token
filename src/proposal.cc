#include "allocator.h"
#include "proposal.h"

namespace Token{
#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    Handle<Proposal> Proposal::NewInstance(uint32_t height, const uint256_t& hash, const std::string& proposer){
        return new Proposal(proposer, hash, height);
    }

    void Proposal::SetPhase(Proposal::Phase phase){
        LOCK;
        phase_ = phase;
        SIGNAL_ALL;
    }

    void Proposal::WaitForPhase(Proposal::Phase phase){
        LOG(INFO) << "waiting for " << phase << " phase....";
        LOCK;
        while(phase_ != phase) WAIT;
    }

    void Proposal::WaitForRequiredVotes(uint32_t required){
        required = required + 10;
        LOG(INFO) << "waiting for " << required << " votes....";
        LOCK;
        while(votes_.size() < required) WAIT;
    }

    void Proposal::WaitForRequiredCommits(uint32_t required){
        LOG(INFO) << "waiting for " << required << " commits....";
        LOCK;
        while(commits_.size() < required) WAIT;
    }

    Proposal::Phase Proposal::GetPhase(){
        LOCK_GUARD;
        return phase_;
    }

    void Proposal::Vote(const std::string& info){
        LOCK_GUARD;
        if(!votes_.insert(info).second) LOG(WARNING) << info << " already voted!";
        SIGNAL_ALL;
    }

    void Proposal::Commit(const std::string& info){
        LOCK_GUARD;
        if(!commits_.insert(info).second) LOG(WARNING) << info << "already committed!";
        SIGNAL_ALL;
    }

    uint32_t Proposal::GetNumberOfVotes(){
        LOCK_GUARD;
        return votes_.size();
    }

    uint32_t Proposal::GetNumberOfCommits(){
        LOCK_GUARD;
        return commits_.size();
    }

    std::string Proposal::ToString() const{
        std::stringstream ss;
        ss << "Proposal(#" << GetHeight() << ")";
        return ss.str();
    }
}