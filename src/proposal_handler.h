#ifndef TOKEN_PROPOSAL_HANDLER_H
#define TOKEN_PROPOSAL_HANDLER_H

#include <memory>
#include "pool.h"
#include "proposal.h"
#include "job/verifier.h"
#include "job/scheduler.h"
#include "job/processor.h"

#ifdef TOKEN_ENABLE_SERVER
#include "peer/peer_session_manager.h"
#endif//TOKEN_ENABLE_SERVER

namespace Token{
  class ProposalHandler{
   protected:
    ProposalPtr proposal_;

    ProposalHandler(const ProposalPtr& proposal):
      proposal_(proposal){}

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
    std::shared_ptr<PeerSession> GetProposer() const{
      return proposal_->GetPeer();
    }
    #endif//TOKEN_ENABLE_SERVER

    Proposal::Result GetResult() const{
      return proposal_->GetResult();
    }

    virtual bool ProcessBlock(const BlockPtr& blk) const = 0;
    virtual bool ProcessProposal() const = 0;
  };

  class NewProposalHandler : public ProposalHandler{
   public:
    NewProposalHandler(const ProposalPtr& proposal):
      ProposalHandler(proposal){}
    ~NewProposalHandler() = default;

    bool ProcessBlock(const BlockPtr& blk) const{
      JobWorker* worker = JobScheduler::GetRandomWorker();
      ProcessBlockJob* job = new ProcessBlockJob(blk, true);
      worker->Submit(job);
      worker->Wait(job);
      return true;
    }

    bool ProcessProposal() const{
      // 1. Voting Phase
      if(!TransitionToPhase(Proposal::kVotingPhase)){
        return false;
      }
      #ifdef TOKEN_ENABLE_SERVER
      PeerSessionManager::BroadcastPrepare();
      #endif//TOKEN_ENABLE_SERVER

      GetProposal()->WaitForRequiredResponses(); // TODO: add timeout here
      if(WasRejected()){
        CancelProposal();
      }

      // 2. Commit Phase
      if(!TransitionToPhase(Proposal::kCommitPhase)){
        return false;
      }
      #ifdef TOKEN_ENABLE_SERVER
      PeerSessionManager::BroadcastCommit();
      #endif//TOKEN_ENABLE_SERVER

      GetProposal()->WaitForRequiredResponses(); //TODO: add timeout here
      if(WasRejected()){
        CancelProposal();
      }

      // 3. Quorum Phase
      CommitProposal();
      BlockDiscoveryThread::ClearProposal();
      return true;
    }
  };

  #ifdef TOKEN_ENABLE_SERVER
  class PeerProposalHandler : public ProposalHandler{
   public:
    PeerProposalHandler(const ProposalPtr& proposal):
      ProposalHandler(proposal){}
    ~PeerProposalHandler() = default;

    bool ProcessBlock(const BlockPtr& blk) const{
      JobWorker* worker = JobScheduler::GetRandomWorker();
      ProcessBlockJob* job = new ProcessBlockJob(blk);
      worker->Submit(job);
      worker->Wait(job);
      return true;
    }

    bool ProcessProposal() const{
      std::shared_ptr<PeerSession> proposer = GetProposer();
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

      JobWorker* worker = JobScheduler::GetRandomWorker();
      VerifyBlockJob* job = new VerifyBlockJob(blk);
      worker->Submit(job);
      worker->Wait(job);

      JobResult& result = job->GetResult();
      if(!result.IsSuccessful()){
        LOG(WARNING) << "block " << hash << " is invalid:";
        LOG(WARNING) << result.GetMessage();
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

      GetProposal()->WaitForPhase(Proposal::kQuorumPhase);
      LOG(INFO) << "proposal " << hash << " has finished, result: " << GetResult();
      return true;
    }
  };
  #endif//TOKEN_ENABLE_SERVER
}

#endif //TOKEN_PROPOSAL_HANDLER_H