#include "proposer.h"
#include "block_queue.h"
#include "proposal.h"
#include "server.h"
#include "message.h"
#include "block_handler.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static Thread::State state_ = Thread::State::kStopped;
    static Proposal* proposal_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define UNLOCK lock.unlock();
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

    static inline Handle<Proposal>
    CreateNewProposal(const Handle<Block>& block){
        if(ProposerThread::HasProposal()){
            return ProposerThread::GetProposal();
        }

        LOG(INFO) << "creating proposal for: " << block->GetHeader();
        Handle<Proposal> proposal = Proposal::NewInstance(block, Server::GetID());
        ProposerThread::SetProposal(proposal);
        return proposal;
    }

#define CHECK_FOR_REJECTION(Proposal) \
    if((Proposal)->IsRejected()){ \
        LOG(WARNING) << "proposal " << (Proposal) << " was rejected, cancelling."; \
        Server::Broadcast(RejectedMessage::NewInstance((Proposal)).CastTo<Message>()); \
        continue; \
    }

#define CHECK_STATUS(ProposalInstance) ({ \
    size_t num_accepted = (ProposalInstance)->GetNumberOfAccepted(); \
    size_t num_rejected = (ProposalInstance)->GetNumberOfRejected(); \
    if(num_rejected > num_accepted) (ProposalInstance)->SetStatus(Proposal::Status::kRejectedStatus); \
})

    void ProposerThread::HandleVotingPhase(const Handle<Proposal>& proposal){
        LOG(INFO) << "proposal " << proposal << " entering voting phase";
        proposal->SetPhase(Proposal::Phase::kVotingPhase);
        Server::Broadcast(PrepareMessage::NewInstance(proposal).CastTo<Message>());
        proposal->WaitForRequiredResponses();
        CHECK_STATUS(proposal);
    }

    void ProposerThread::HandleCommitPhase(const Handle<Proposal>& proposal){
        LOG(INFO) << "proposal " << proposal << " entering commit phase";
        proposal->SetPhase(Proposal::Phase::kCommitPhase);
        Server::Broadcast(CommitMessage::NewInstance(proposal).CastTo<Message>());
        proposal->WaitForRequiredResponses();
        CHECK_STATUS(proposal);
    }

    void ProposerThread::HandleQuorumPhase(const Handle<Proposal>& proposal){
        LOG(INFO) << "proposal " << proposal << " has reached a quorum";
        proposal->SetPhase(Proposal::Phase::kQuorumPhase);
        if(!proposal->IsRejected()){
            LOG(INFO) << "proposal " << proposal << " was accepted";
            proposal->SetStatus(Proposal::Status::kAcceptedStatus);
        }
    }

    void ProposerThread::HandleThread(uword parameter){
        LOG(INFO) << "starting proposal thread....";
        SetState(Thread::State::kRunning);
        while(!IsStopped()){
            if(IsPaused()){
                LOG(INFO) << "pausing proposer thread....";

                LOCK;
                while(GetState() != Thread::State::kRunning || GetState() != Thread::State::kStopped) WAIT;
                UNLOCK;

                if(IsStopped()){
                    LOG(WARNING) << "proposal thread has been halted, exiting.";
                    return;
                }

                LOG(INFO) << "proposal thread is resuming.";
            }

            Handle<Block> block = BlockQueue::DeQueue();
            Handle<Proposal> proposal = CreateNewProposal(block);

            HandleVotingPhase(proposal);
            CHECK_FOR_REJECTION(proposal);

            HandleCommitPhase(proposal);
            CHECK_FOR_REJECTION(proposal);

            HandleQuorumPhase(proposal);
            CHECK_FOR_REJECTION(proposal);

            if(proposal->IsAccepted()){
                uint256_t hash = block->GetSHA256Hash();
                if(!BlockHandler::ProcessBlock(block, true)){
                    LOG(WARNING) << "couldn't process block: " << hash;
                    return;
                }
                BlockPool::RemoveBlock(hash);
                BlockChain::Append(block);
            }
        }
    }

    Thread::State ProposerThread::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void ProposerThread::SetState(Thread::State state){
        LOCK_GUARD;
        state_ = state;
    }

    void ProposerThread::WaitForState(Thread::State state){
        LOCK;
        while(state_ != state) WAIT;
    }
}