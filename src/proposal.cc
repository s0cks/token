#include "allocator.h"
#include "proposal.h"

namespace Token{
#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()
    Handle<Proposal> Proposal::NewInstance(const Handle<Buffer>& buff){
        uint32_t height = buff->GetInt();
        Hash hash = buff->GetHash();
        UUID proposer(buff);
        return new Proposal(proposer, hash, height);
    }

    Handle<Proposal> Proposal::NewInstance(uint32_t height, const Hash& hash, const UUID& proposer){
        return new Proposal(proposer, hash, height);
    }

    size_t Proposal::GetBufferSize() const{
        size_t size = 0;
        size += sizeof(uint32_t);
        size += Hash::kSize;
        size += UUID::kSize;
        return size;
    }

    bool Proposal::Encode(const Handle<Buffer>& buff) const{
        buff->PutInt(height_);
        buff->PutHash(hash_);
        proposer_.Write(buff);
        return true;
    }

    void Proposal::SetPhase(Proposal::Phase phase){
        LOCK;
        phase_ = phase;
        accepted_.clear();
        rejected_.clear();
        UNLOCK;
        SIGNAL_ALL;
    }

    void Proposal::SetStatus(Proposal::Status status){
        LOCK;
        status_ = status;
        UNLOCK;
        SIGNAL_ALL;
    }

    void Proposal::WaitForPhase(Proposal::Phase phase){
        LOG(INFO) << "waiting for " << phase << " phase....";
        LOCK;
        while(phase_ != phase) WAIT;
    }

    void Proposal::WaitForStatus(Proposal::Status status){
        LOG(INFO) << "waiting for " << status << " status....";
        LOCK;
        while(status_ != status) WAIT;
    }

    void Proposal::WaitForRequiredResponses(uint32_t required){
        LOG(INFO) << "waiting for " << required << " peers....";

        LOCK;
        while(GetNumberOfResponses() < required) WAIT;
    }

    size_t Proposal::GetNumberOfAccepted(){
        LOCK_GUARD;
        return accepted_.size();
    }

    size_t Proposal::GetNumberOfRejected(){
        LOCK_GUARD;
        return rejected_.size();
    }

    size_t Proposal::GetNumberOfResponses(){
        LOCK_GUARD;
        return accepted_.size() + rejected_.size();
    }

    Proposal::Phase Proposal::GetPhase(){
        LOCK_GUARD;
        return phase_;
    }

    Proposal::Status Proposal::GetStatus(){
        LOCK_GUARD;
        return status_;
    }

    void Proposal::AcceptProposal(const std::string& node){
        if(HasResponseFrom(node)){
            LOG(WARNING) << "node " << node << "has voted already! skipping second vote.";
            return;
        }

        LOCK_GUARD;
        if(!accepted_.insert(node).second) LOG(WARNING) << "couldn't accept acceptance vote from node: " << node;
        SIGNAL_ALL;
    }

    void Proposal::RejectProposal(const std::string& node){
        if(HasResponseFrom(node)){
            LOG(WARNING) << "node " << node << " has voted already! skipping second vote.";
            return;
        }

        LOCK_GUARD;
        if(!rejected_.insert(node).second) LOG(WARNING) << "couldn't accept rejection vote from node: " << node;
        SIGNAL_ALL;
    }

    bool Proposal::HasResponseFrom(const std::string& node){
        LOCK_GUARD;
        return accepted_.find(node) != accepted_.end() ||
                rejected_.find(node) != rejected_.end();
    }

    std::string Proposal::ToString() const{
        std::stringstream ss;
        ss << "Proposal(#" << GetHeight() << ")";
        return ss.str();
    }
}