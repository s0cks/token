#include "server.h"
#include "proposal.h"
#include "peer/peer_session_manager.h"

namespace Token{
#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    std::shared_ptr<PeerSession> Proposal::GetPeer() const{
        return PeerSessionManager::GetSession(GetProposer());
    }

    Proposal::Phase Proposal::GetPhase(){
        LOCK_GUARD;
        return phase_;
    }

    void Proposal::SetPhase(const Proposal::Phase& phase){
        LOCK;
        phase_ = phase;
        accepted_.clear();
        rejected_.clear();
        UNLOCK;
        SIGNAL_ALL;
    }

    void Proposal::WaitForPhase(const Phase& phase){
        LOCK;
        while(phase_ != phase) WAIT;
        UNLOCK;
    }

    Proposal::Result Proposal::GetResult(){
        LOCK_GUARD;
        return result_;
    }

    void Proposal::SetStatus(const Proposal::Result& result){
        LOCK;
        result_ = result;
        UNLOCK;
        SIGNAL_ALL;
    }

    void Proposal::WaitForResult(const Result& result){
        LOCK;
        while(result_ != result) WAIT;
        UNLOCK;
    }

    void Proposal::WaitForRequiredResponses(int required){
        LOG(INFO) << "waiting for " << required << " peers....";

        LOCK;
        while((int)(accepted_.size() + rejected_.size()) < required) WAIT;
        UNLOCK;
    }

    int64_t Proposal::GetNumberOfAccepted(){
        LOCK_GUARD;
        return accepted_.size();
    }

    int64_t Proposal::GetNumberOfRejected(){
        LOCK_GUARD;
        return rejected_.size();
    }

    int64_t Proposal::GetNumberOfResponses(){
        LOCK_GUARD;
        return accepted_.size() + rejected_.size();
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

    int Proposal::GetRequiredNumberOfPeers(){
        int32_t peers = PeerSessionManager::GetNumberOfConnectedPeers();
        if(peers == 0) return 0;
        else if(peers == 1) return 1;
        return peers / 2;
    }
}