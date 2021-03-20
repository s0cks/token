#include <mutex>

#include "pool.h"
#include "miner.h"
#include "block_builder.h"
#include "job/scheduler.h"
#include "job/job_proposal.h"

namespace token{
#define MINER_LOG(LevelName) \
  LOG(LevelName) << "[miner] "

  BlockMiner* BlockMiner::GetInstance(){
    static BlockMiner instance;
    return &instance;
  }

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

  void BlockMiner::OnMine(uv_timer_t* handle){
    BlockMiner* miner = (BlockMiner*)handle->data;
    if(miner->HasActiveProposal()){
      MINER_LOG(WARNING) << "mine called during active proposal.";
      return; // skip
    }

    ObjectPoolPtr pool = ObjectPool::GetInstance();
    if(pool->GetNumberOfTransactions() == 0){
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
    ProposalPtr proposal = Proposal::NewInstance(miner->GetLoop(), blk, GetRequiredVotes());
    if(!miner->SetActiveProposal(proposal)){
      MINER_LOG(ERROR) << "cannot set active proposal.";
      return;
    }

    ProposalJob* job = new ProposalJob(proposal);
    if(!JobScheduler::Schedule(job)){
      MINER_LOG(ERROR) << "cannot schedule ProposalJob";
      return;
    }
    //TODO: free job
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