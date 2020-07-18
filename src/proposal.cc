#include "allocator.h"
#include "proposal.h"

namespace Token{
    static std::mutex pmutex_;
    static Proposal* proposal_ = nullptr;

#define PLOCK_GUARD std::lock_guard<std::mutex> pguard(pmutex_)
#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    Proposal* Proposal::NewInstance(const uint256_t& hash, const NodeInfo& proposer){
        Proposal* instance = (Proposal*)Allocator::Allocate(sizeof(Proposal)); //TODO: Type proposal
        new (instance)Proposal(proposer, hash);
        return instance;
    }

    void Proposal::SetPhase(Proposal::Phase phase){
        LOCK;
        phase_ = phase;
        SIGNAL_ALL;
    }

    void Proposal::WaitForPhase(Proposal::Phase phase){
        LOCK;
        while(phase_ != phase) WAIT;
    }

    void Proposal::WaitForRequiredVotes(uint32_t required){
        LOCK;
        while(votes_.size() < required) WAIT;
    }

    void Proposal::WaitForRequiredCommits(uint32_t required){
        LOCK;
        while(commits_.size() < required) WAIT;
    }

    Proposal::Phase Proposal::GetPhase(){
        LOCK_GUARD;
        return phase_;
    }

    void Proposal::Vote(const NodeInfo& info){
        LOCK_GUARD;
        if(!votes_.insert(info).second) LOG(WARNING) << info << " already voted!";
    }

    void Proposal::Commit(const NodeInfo& info){
        LOCK_GUARD;
        if(!commits_.insert(info).second) LOG(WARNING) << info << "already committed!";
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
        ss << "Proposal(" << hash_ << ")";
        return ss.str();
    }
}