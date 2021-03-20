#ifndef TOKEN_JOB_PROPOSAL_H
#define TOKEN_JOB_PROPOSAL_H

#include "job/job.h"
#include "consensus/proposal.h"

namespace token{
  class ProposalJob : public Job{
   private:
    ProposalPtr proposal_;
   protected:
    bool ExecutePhase1(){
      // Phase 1 (Prepare)
      //TODO: add timestamps
#ifdef TOKEN_DEBUG
      JOB_LOG(INFO, this) << "executing proposal phase 1....";
#endif//TOKEN_DEBUG
      // transition the proposal to phase 1
      if(!proposal_->TransitionToPhase(ProposalPhase::kPreparePhase)){
        //TODO: cancel proposal
        return false;
      }

      Phase1Quorum& quorum = proposal_->GetPhase1Quorum();
      // start phase 1 timer
#ifdef TOKEN_DEBUG
      JOB_LOG(INFO, this) << "starting phase 1 quorum timer....";
#endif//TOKEN_DEBUG
      quorum.StartTimer();

      // broadcast prepare to quorum members
      PeerSessionManager::BroadcastPrepare();

      // wait for the required votes to return from peers
#ifdef TOKEN_DEBUG
      JOB_LOG(INFO, this) << "waiting for required votes....";
#endif//TOKEN_DEBUG
      quorum.WaitForRequiredVotes();

      // stop the phase 1 timer
#ifdef TOKEN_DEBUG
      JOB_LOG(INFO, this) << "stopping phase 1 quorum timer....";
#endif//TOKEN_DEBUG
      quorum.StopTimer();

      // check quorum results.
      QuorumResult result = quorum.GetResult();
#ifdef TOKEN_DEBUG
      JOB_LOG(INFO, this) << "phase 1 result: " << result;
#endif//TOKEN_DEBUG
      return true;
    }

    bool ExecutePhase2(){
      // Phase 2 (Commit)
      //TODO: add timestamps
#ifdef TOKEN_DEBUG
      JOB_LOG(INFO, this) << "executing proposal phase 2....";
#endif//TOKEN_DEBUG

      // transition the proposal to phase 2
      if(!proposal_->TransitionToPhase(ProposalPhase::kCommitPhase))
        return false;

      Phase2Quorum& quorum = proposal_->GetPhase2Quorum();
      // start phase 2 timer
      quorum.StartTimer();

      // broadcast commit to quorum members
      PeerSessionManager::BroadcastCommit();

      // wait for the required votes to return from peers
      quorum.WaitForRequiredVotes();

      // stop phase 2 timer
      quorum.StopTimer();

      // check quorum results.
      QuorumResult result = quorum.GetResult();
#ifdef TOKEN_DEBUG
      JOB_LOG(INFO, this) << "phase 2 result: " << result;
#endif//TOKEN_DEBUG

      // cleanup
      return true;
    }

    JobResult DoWork(){
      BlockMiner* miner = BlockMiner::GetInstance();

      JOB_LOG(INFO, this) << "pausing the miner....";
      if(!miner->Pause())
        return Failed("Cannot pause the miner.");

      if(!ExecutePhase1())
        return Failed("Proposal failed during phase 1.");

//      if(!ExecutePhase2())
//        return Failed("Proposal failed during phase 2.");
      return Success("Proposal has finished.");
    }
   public:
    ProposalJob(const ProposalPtr& proposal):
      Job(nullptr, "ProposalJob"),
      proposal_(proposal){}
    ~ProposalJob() = default;
  };
}

#endif//TOKEN_JOB_PROPOSAL_H