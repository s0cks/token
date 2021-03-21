#include <mutex>

#include "pool.h"
#include "miner.h"
#include "block_builder.h"
#include "job/scheduler.h"

namespace token{
  static JobQueue queue_(JobScheduler::kMaxNumberOfJobs);

  static inline bool
  Schedule(Job* job){
    return queue_.Push(job);
  }

  static inline bool
  RegisterQueue(){
    return JobScheduler::RegisterQueue(pthread_self(), &queue_);
  }

  BlockMiner* BlockMiner::GetInstance(){
    static BlockMiner instance;
    return &instance;
  }

  bool BlockMiner::StartMiningTimer(){
    DLOG_MINER(INFO) << "starting timer.";
    VERIFY_UVRESULT(uv_timer_start(&timer_, &OnMine, FLAGS_mining_interval, FLAGS_mining_interval), DLOG_MINER(ERROR), "cannot start timer");
    return true;
  }

  bool BlockMiner::StopMiningTimer(){
    DLOG_MINER(INFO) << "stopping timer.";
    VERIFY_UVRESULT(uv_timer_stop(&timer_), DLOG_MINER(ERROR), "cannot stop timer");
    return true;
  }

  static inline void
  CancelProposal(const ProposalPtr& proposal){
    DLOG_MINER(INFO) << "cancelling proposal: " << proposal->raw();
  }

  void BlockMiner::OnMine(uv_timer_t* handle){
    auto miner = (BlockMiner*)handle->data;
    if(miner->HasActiveProposal()){
      DLOG_MINER(WARNING) << "mine called during active proposal.";
      return; // skip
    }

    ObjectPoolPtr pool = ObjectPool::GetInstance();
    if(pool->GetNumberOfTransactions() == 0){
      DLOG_MINER(WARNING) << "skipping mining cycle, no transactions in pool.";
      return; // skip
    }

    // create a new block
    DLOG_MINER(INFO) << "creating new block....";
    BlockPtr blk = BlockBuilder::BuildNewBlock();
    DLOG_MINER(INFO) << "discovered new block: " << blk->GetHash();

    // create a new proposal
    ProposalPtr proposal = Proposal::NewInstance(miner->GetLoop(), blk, GetRequiredVotes());
    if(!miner->SetActiveProposal(proposal)){
      LOG_MINER(ERROR) << "cannot set active proposal.";
      return;
    }

    auto job = new ProposalJob(miner, proposal);//TODO: memory-leak
    if(!Schedule(job)){
      LOG_MINER(ERROR) << "cannot schedule new job.";
      return;
    }
  }

  bool BlockMiner::Run(){
    if(IsRunning())
      return false;

    DLOG_MINER(INFO) << "starting....";
    SetState(BlockMiner::kStartingState);
    VERIFY_UVRESULT(uv_timer_init(loop_, &timer_), LOG_MINER(ERROR), "cannot initialize the miner timer");

    if(!StartMiningTimer()){
      LOG_MINER(ERROR) << "cannot start timer.";
      SetState(BlockMiner::kStoppedState);
      return false;
    }

    DLOG_MINER(INFO) << "running....";
    SetState(BlockMiner::kRunningState);
    VERIFY_UVRESULT(uv_run(loop_, UV_RUN_DEFAULT), LOG_MINER(ERROR), "cannot run block miner loop");

    SetState(BlockMiner::kStoppedState);
    DLOG_MINER(INFO) << "stopped.";
    return true;
  }

  static ThreadId thread_;

  bool BlockMinerThread::Stop(){
    return ThreadJoin(thread_);
  }

  bool BlockMinerThread::Start(){
    return ThreadStart(&thread_, BlockMiner::GetThreadName(), &HandleThread, (uword)BlockMiner::GetInstance());
  }

  void BlockMinerThread::HandleThread(uword param){
    if(!RegisterQueue()){
      LOG_MINER(ERROR) << "cannot register job queue.";
      return;
    }

    BlockMiner* miner = (BlockMiner*)param;
    LOG_MINER_IF(ERROR, !miner->Run()) << "cannot run miner loop.";
  }
}