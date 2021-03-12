#include <mutex>
#include <condition_variable>

#include "pool.h"
#include "miner.h"
#include "block_builder.h"
#include "job/scheduler.h"
#include "peer/peer_session_manager.h"

namespace token{
  BlockMiner* BlockMiner::GetInstance(){
    static BlockMiner instance;
    return &instance;
  }

#define MINER_LOG(LevelName) \
  LOG(LevelName) << "[Miner] "

  bool BlockMiner::StartMiningTimer(){
#ifdef TOKEN_DEBUG
    MINER_LOG(INFO) << "starting timer....";
#endif//TOKEN_DEBUG
    VERIFY_UVRESULT(uv_timer_start(&timer_, &OnMine, FLAGS_miner_interval, FLAGS_miner_interval), MINER_LOG(ERROR), "cannot start timer");
    return true;
  }

  bool BlockMiner::StopMiningTimer(){
#ifdef TOKEN_DEBUG
    MINER_LOG(INFO) << "stopping timer....";
#endif//TOKEN_DEBUG
    VERIFY_UVRESULT(uv_timer_stop(&timer_), MINER_LOG(ERROR), "cannot stop timer");
    return true;
  }

  static inline void
  CancelProposal(const ProposalPtr& proposal){
    MINER_LOG(INFO) << "cancelling proposal: " << proposal->raw();
  }

  static inline void
  ExecuteProposalPhase1(const ProposalPtr& proposal){
    // Phase 1 (Prepare)
    //TODO: add timestamps

#ifdef TOKEN_DEBUG
    MINER_LOG(INFO) << "executing proposal phase 1....";
#endif//TOKEN_DEBUG
    // transition the proposal to phase 1
    if(!proposal->TransitionToPhase(ProposalPhase::kPreparePhase))
      return CancelProposal(proposal);

    Phase1Quorum& quorum = proposal->GetPhase1Quorum();
    // start phase 1 timer
    quorum.StartTimer();

    // broadcast prepare to quorum members
    PeerSessionManager::BroadcastPrepare();

    // wait for the required votes to return from peers
    quorum.WaitForRequiredVotes();

    // stop the phase 1 timer
    quorum.StopTimer();

    // check quorum results.
    QuorumResult result = quorum.GetResult();
#ifdef TOKEN_DEBUG
    MINER_LOG(INFO) << "phase 1 result: " << result;
#endif//TOKEN_DEBUG
  }

  static inline void
  ExecuteProposalPhase2(const ProposalPtr& proposal){
    // Phase 2 (Commit)
    //TODO: add timestamps

#ifdef TOKEN_DEBUG
    MINER_LOG(INFO) << "executing proposal phase 2....";
#endif//TOKEN_DEBUG

    // transition the proposal to phase 2
    if(!proposal->TransitionToPhase(ProposalPhase::kCommitPhase))
      return CancelProposal(proposal);

    Phase2Quorum& quorum = proposal->GetPhase2Quorum();
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
    MINER_LOG(INFO) << "phase 2 result: " << result;
#endif//TOKEN_DEBUG

    // cleanup

  }

  static inline void
  ExecuteProposal(const ProposalPtr& proposal){
    ExecuteProposalPhase1(proposal);
    ExecuteProposalPhase2(proposal);
    // commit block to block chain
  }

  void BlockMiner::OnMine(uv_timer_t* handle){
    BlockMiner* miner = (BlockMiner*)handle->data;
    if(miner->HasActiveProposal()){
      MINER_LOG(WARNING) << "mine called during active proposal.";
      return; // skip
    }

    if(ObjectPool::GetNumberOfTransactions() == 0){
      MINER_LOG(WARNING) << "no transactions in object pool, skipping mining cycle.";
      return; // skip
    }

#ifdef TOKEN_DEBUG
    MINER_LOG(INFO) << "mining block....";
#endif//TOKEN_DEBUG

    // pause the miner
    if(!miner->Pause())
      MINER_LOG(ERROR) << "cannot pause miner.";

    // create a new block
    BlockPtr blk = BlockBuilder::BuildNewBlock();
#ifdef TOKEN_DEBUG
    MINER_LOG(INFO) << "discovered block: " << blk->GetHeader();
#else
    MINER_LOG(INFO) << "discovered block: " << blk->GetHash();
#endif//TOKEN_DEBUG

    // create a new proposal
    ProposalPtr proposal = Proposal::NewInstance(nullptr, miner->GetLoop(), blk, GetRequiredVotes());
    if(!miner->SetActiveProposal(proposal)){
      MINER_LOG(ERROR) << "cannot set active proposal.";
      return;
    }

    // execute the proposal
    ExecuteProposal(proposal);
    // clear the active proposal
    if(!miner->ClearActiveProposal())
      MINER_LOG(WARNING) << "cannot clear the active proposal.";
    // resume mining
    if(!miner->Resume())
      MINER_LOG(WARNING) << "cannot start mining timer.";
  }

  bool BlockMiner::Run(){
    if(IsRunning())
      return false;

#ifdef TOKEN_DEBUG
    MINER_LOG(INFO) << "starting....";
#endif//TOKEN_DEBUG
    SetState(BlockMiner::kStartingState);
    VERIFY_UVRESULT(uv_timer_init(loop_, &timer_), MINER_LOG(ERROR), "cannot initialize the miner timer");

    if(!StartMiningTimer())
      MINER_LOG(WARNING) << "cannot start miner timer.";

#ifdef TOKEN_DEBUG
    MINER_LOG(INFO) << "running....";
#endif//TOKEN_DEBUG
    SetState(BlockMiner::kRunningState);
    VERIFY_UVRESULT(uv_run(loop_, UV_RUN_DEFAULT), MINER_LOG(ERROR), "cannot run block miner loop");

    SetState(BlockMiner::kStoppedState);
#ifdef TOKEN_DEBUG
    MINER_LOG(INFO) << "stopped.";
#endif//TOKEN_DEBUG
    return true;
  }

  static ThreadId thread_;
  static JobQueue queue_(JobScheduler::kMaxNumberOfJobs);

  void BlockMinerThread::Initialize(){
    if(!JobScheduler::RegisterQueue(thread_, &queue_)){
      MINER_LOG(ERROR) << "cannot register job queue.";
      return;
    }
  }

  bool BlockMinerThread::Stop(){
    return ThreadJoin(thread_);
  }

  bool BlockMinerThread::Start(){
    return ThreadStart(&thread_, "miner", &HandleThread, 0);
  }

  void BlockMinerThread::HandleThread(uword param){
    BlockMiner miner;
    if(!miner.Run())
      LOG(WARNING) << "cannot run block miner.";
  }
}