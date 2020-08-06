#include "proposer.h"
#include "block_queue.h"
#include "proposal.h"
#include "server.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static Proposal* proposal_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    bool ProposerThread::HasProposal(){
        LOCK_GUARD;
        return proposal_ != nullptr;
    }

    bool ProposerThread::SetProposal(const Handle<Proposal>& proposal){
        if(HasProposal()){
            LOG(WARNING) << "you can only have one active proposal at any given time";
            return false;
        }

        LOCK_GUARD;
        WriteBarrier(&proposal_, proposal);
        return true;
    }

    Handle<Proposal> ProposerThread::GetProposal(){
        LOCK_GUARD;
        return proposal_;
    }

    void ProposerThread::HandleThread(uword parameter){
        LOG(INFO) << "starting proposal thread....";
        Handle<Block> block = BlockQueue::DeQueue();
        Handle<Proposal> proposal = Proposal::NewInstance(block, Server::GetID());
        LOG(INFO) << "creating proposal: " << proposal;
    }
}