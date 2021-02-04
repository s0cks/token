#ifndef TOKEN_PROPOSAL_HANDLER_H
#define TOKEN_PROPOSAL_HANDLER_H

#include <memory>
#include "pool.h"
#include "job/verifier.h"
#include "job/scheduler.h"
#include "job/processor.h"

#include "consensus/proposal.h"
#include "consensus/proposal_manager.h"

#ifdef TOKEN_ENABLE_SERVER
#include "peer/peer_session_manager.h"
#endif//TOKEN_ENABLE_SERVER

namespace token{
  class ProposalHandler{
   protected:
    JobQueue& queue_;
    ProposalPtr proposal_;

    ProposalHandler(JobQueue& queue, const ProposalPtr& proposal):
      queue_(queue),
      proposal_(proposal){}

    bool ScheduleSnapshot() const{
      LOG(INFO) << "scheduling new snapshot....";
      LOG(WARNING) << "snapshots have been disabled.";
//      SnapshotJob* job = new SnapshotJob();
//      if(!JobScheduler::Schedule(job))
//        LOG(WARNING) << "couldn't schedule new snapshot.";
      return false;
    }

    bool ProcessBlock(const BlockPtr& blk) const{
      ProcessBlockJob* job = new ProcessBlockJob(blk, true);
      queue_.Push(job);
      while(!job->IsFinished()); //spin

      if(!job->GetResult().IsSuccessful()){
        LOG(ERROR) << "ProcessBlockJob finished w/: " << job->GetResult();
        return false;
      }

      if(!job->CommitAllChanges()){
        LOG(ERROR) << "couldn't commit changes to block chain.";
        return false;
      }

      delete job;
      return true;
    }

    bool WasRejected() const;
    bool CommitProposal() const;
    bool CancelProposal() const;
    bool TransitionToPhase(const Proposal::Phase& phase) const;
   public:
    virtual ~ProposalHandler() = default;

    ProposalPtr GetProposal() const{
      return proposal_;
    }

    int64_t GetProposalID() const{
      return proposal_->GetHeight();
    }

    Hash GetProposalHash() const{
      return proposal_->GetHash();
    }

    #ifdef TOKEN_ENABLE_SERVER
    PeerSession* GetProposer() const{
      return proposal_->GetPeer();
    }
    #endif//TOKEN_ENABLE_SERVER

    Proposal::Result GetResult() const{
      return proposal_->GetResult();
    }

    virtual bool ProcessProposal() const = 0;
  };

  class NewProposalHandler : public ProposalHandler{
   public:
    NewProposalHandler(JobQueue& queue, const ProposalPtr& proposal):
      ProposalHandler(queue, proposal){}
    ~NewProposalHandler() = default;

    bool ProcessProposal() const{
    #ifdef TOKEN_ENABLE_SERVER
      PeerSessionManager::BroadcastPrepare();
    #endif//TOKEN_ENABLE_SERVER

      GetProposal()->WaitForRequiredResponses();//TODO: add timeout here
      if(WasRejected())
        return CancelProposal();


      // 1. Voting Phase
      if(!TransitionToPhase(Proposal::kVotingPhase))
        return false;
    #ifdef TOKEN_ENABLE_SERVER
      PeerSessionManager::BroadcastPromise();
    #endif//TOKEN_ENABLE_SERVER

      GetProposal()->WaitForRequiredResponses(); // TODO: add timeout here
      if(WasRejected())
        return CancelProposal();

      // 2. Commit Phase
      if(!TransitionToPhase(Proposal::kCommitPhase))
        return false;
    #ifdef TOKEN_ENABLE_SERVER
      PeerSessionManager::BroadcastCommit();
    #endif//TOKEN_ENABLE_SERVER

      GetProposal()->WaitForRequiredResponses(); //TODO: add timeout here
      if(WasRejected())
        return CancelProposal();

      // 3. Quorum Phase
      CommitProposal();
      return ProposalManager::ClearProposal();
    }
  };

  #ifdef TOKEN_ENABLE_SERVER
  class PeerProposalHandler : public ProposalHandler{
   public:
    PeerProposalHandler(JobQueue& queue, const ProposalPtr& proposal):
      ProposalHandler(queue, proposal){}
    ~PeerProposalHandler() = default;

    bool ProcessProposal() const{
      PeerSession* proposer = GetProposer();
      Hash hash = GetProposalHash();
      if(!ObjectPool::HasBlock(hash)){
        //TODO: fix requesting of data
        LOG(INFO) << hash << " cannot be found, requesting block from peer: " << proposer->GetID();
        std::vector<InventoryItem> items = {
          InventoryItem(InventoryItem::kBlock, hash)
        };
        GetProposer()->Send(GetDataMessage::NewInstance(items));
        if(!ObjectPool::WaitForBlock(hash)){
          LOG(INFO) << "cannot resolve block " << hash << ", rejecting....";
          return CancelProposal();
        }
      }

      BlockPtr blk = ObjectPool::GetBlock(hash);
      LOG(INFO) << "proposal " << hash << " has entered the voting phase.";

      if(!ProcessBlock(blk)){
        LOG(WARNING) << "couldn't process block: " << hash;
        return CancelProposal();
      }
      proposer->SendAccepted();

      GetProposal()->WaitForPhase(Proposal::kCommitPhase);
      LOG(INFO) << "proposal " << hash << " has entered the commit phase.";
      if(!CommitProposal()){
        LOG(ERROR) << "couldn't commit proposal #" << GetProposalID() << ", rejecting....";
        return CancelProposal();
      }
      proposer->SendAccepted();
      LOG(INFO) << "proposal " << hash << " has finished, result: " << GetResult();
      return ProposalManager::ClearProposal();
    }
  };
  #endif//TOKEN_ENABLE_SERVER
}

#endif //TOKEN_PROPOSAL_HANDLER_H