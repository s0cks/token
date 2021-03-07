#include <mutex>
#include <condition_variable>

#include "pool.h"
#include "miner.h"
#include "rpc/rpc_server.h"
#include "block_builder.h"
#include "consensus/proposal_handler.h"
#include "peer/peer_session_manager.h"
#include "consensus/proposal_manager.h"

namespace token{
#define MINER_LOG(LevelName) \
  LOG(LevelName) << "[Miner] "

#define CHECK_UVRESULT(Result, LevelName, Message) \
  if((err = Result) != 0){                         \
    MINER_LOG(LevelName) << Message << ": " << uv_strerror(err); \
    return false;                                  \
  }

  bool BlockMiner::StartTimer(){
    int err;
    CHECK_UVRESULT(uv_timer_start(&timer_, &OnMine, FLAGS_miner_interval, FLAGS_miner_interval), ERROR, "cannot start timer");
    return true;
  }

  bool BlockMiner::StopTimer(){
    int err;
    CHECK_UVRESULT(uv_timer_stop(&timer_), ERROR, "cannot stop timer");
    return true;
  }

  void BlockMiner::OnMine(uv_timer_t* handle){
    if(ProposalManager::HasProposal()){
      LOG(WARNING) << "mine called during active proposal.";
      return;
    }

    if(ObjectPool::GetNumberOfTransactions() == 0){
      LOG(WARNING) << "no transactions in object pool, skipping mining cycle.";
      return; // skip
    }

    BlockMiner* miner = (BlockMiner*)handle->data;
    BlockPtr blk = BlockBuilder::BuildNewBlock();
    Hash hash = blk->GetHash();

    LOG(INFO) << "discovered block " << hash << ", creating proposal....";
    ProposalPtr proposal = Proposal::NewInstance(miner->GetLoop(), blk);
    if(!ProposalManager::SetProposal(proposal)){
      LOG(WARNING) << "cannot set active proposal to: " << proposal->ToString() << ", abandoning proposal.";
      return;
    }

#ifdef TOKEN_ENABLE_SERVER
    PeerSessionManager::BroadcastDiscoveredBlock();
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_SERVER
    // 1. prepare phase
    PeerSessionManager::BroadcastPrepare();
    proposal->WaitForRequiredResponses();
#endif//TOKEN_ENABLE_SERVER

//#ifdef TOKEN_ENABLE_SERVER
//    // 3. commit phase
//    if(!TransitionToPhase(Proposal::kCommitPhase))
//      return CancelProposal();
//    PeerSessionManager::BroadcastCommit();
//    proposal->WaitForRequiredResponses();
//    if(WasRejecte())
//      return CancelProposal();
//#endif//TOKEN_ENABLE_SERVER
//
//#ifdef TOKEN_ENABLE_SERVER
//    // 4. quorum phase
//    CommitProposal();
//#endif//TOKEN_ENABLE_SERVER
  }

  bool BlockMiner::Run(){
    if(IsRunning())
      return false;

    SetState(BlockMiner::kStartingState);
    if(!StartTimer())
      LOG(WARNING) << "cannot start miner timer.";
    SetState(BlockMiner::kRunningState);
    int err;
    CHECK_UVRESULT(uv_run(loop_, UV_RUN_DEFAULT), ERROR, "cannot run block miner loop");
    SetState(BlockMiner::kStoppedState);
    return true;
  }

//  static inline JobResult
//  ProcessBlock(const BlockPtr& blk){
//    ProcessBlockJob* job = new ProcessBlockJob(blk, true);
//    queue_.Push(job);
//    while(!job->IsFinished()); //spin
//    JobResult result(job->GetResult());
//    delete job;
//    return result;
//  }

//  bool BlockMiner::ScheduleSnapshot(){
//    LOG(INFO) << "scheduling new snapshot....";
//    LOG(WARNING) << "snapshots have been disabled.";
//    SnapshotJob* job = new SnapshotJob();
//    if(!JobScheduler::Schedule(job))
//      LOG(WARNING) << "couldn't schedule new snapshot.";
//    return false;
//  }

//  bool BlockMiner::Commit(const ProposalPtr& proposal){
//    if(!proposal->TransitionToPhase(Proposal::kQuorumPhase))
//      return false;
//
//    Hash hash = proposal->GetHash();
//    BlockPtr blk = ObjectPool::GetBlock(hash);
//
//    JobResult result = ProcessBlock(blk);
//    if(!result.IsSuccessful()){
//      LOG(WARNING) << "couldn't process block " << hash << ": " << result.GetMessage();
//      return false;
//    }
//
//    if(!BlockChain::Append(blk)){
//      LOG(WARNING) << "couldn't append block " << hash << ".";
//      return false;
//    }
//
//    if(!ObjectPool::RemoveBlock(hash)){
//      LOG(WARNING) << "couldn't remove block " << hash << " from pool.";
//      return false;
//    }
//
//    if(FLAGS_enable_snapshots && !ScheduleSnapshot())
//      LOG(WARNING) << "couldn't schedule new snapshot.";
//
//    LOG(INFO) << "proposal " << proposal->ToString() << " has finished.";
//    return BlockMiner::Resume();
//  }

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