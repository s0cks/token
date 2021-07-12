#include <mutex>

#include "pool.h"
#include "miner.h"
#include "block_builder.h"
#include "job/scheduler.h"
#include "job/processor.h"

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

  bool BlockMiner::SendAccepted(){
    DLOG_MINER(INFO) << "sending accepted....";
    VERIFY_UVRESULT(uv_async_send(&on_accepted_), DLOG_MINER(ERROR), "cannot send accepted");
    return true;
  }

  bool BlockMiner::SendRejected(){
    DLOG_MINER(INFO) << "sending rejected....";
    VERIFY_UVRESULT(uv_async_send(&on_rejected_), DLOG_MINER(ERROR), "cannot send rejected");
    return true;
  }

  static inline void
  CancelProposal(const ProposalPtr& proposal){
    DLOG_MINER(INFO) << "cancelling proposal: " << proposal->raw();
  }

  void BlockMiner::OnAccepted(uv_async_t *handle){
    DLOG_MINER(INFO) << "OnAccepted Received";

    auto miner = (BlockMiner*)handle->data;
    auto chain = BlockChain::GetInstance();
    auto pool = ObjectPool::GetInstance();
    if(!miner->HasActiveProposal()){
      DLOG_MINER(ERROR) << "there is no active proposal";
      return;
    }

    //TODO:
    // - sanity check proposals for equivalency
    // - sanity check proposal state
    ProposalPtr proposal = miner->GetActiveProposal();
    DLOG_MINER(INFO) << "proposal " << proposal->raw() << " was accepted by peers.";

    BlockHeader& header = proposal->raw().value();
    Hash hash = header.hash();
    DLOG_MINER(INFO) << "appending block: " << header;

    BlockPtr blk = pool->GetBlock(hash);
    auto job = new ProcessBlockJob(blk, true);
    DLOG_MINER_IF(ERROR, !Schedule(job)) << "cannot schedule process block job.";
    while(!job->IsFinished()); //spin

    DLOG_MINER_IF(ERROR, !chain->Append(blk)) << "cannot append the new block.";
    DLOG_MINER_IF(ERROR, !miner->ClearActiveProposal()) << "cannot clear active proposal";
    DLOG_MINER_IF(ERROR, !miner->StartMiningTimer()) << "cannot start the mining timer.";
  }

  void BlockMiner::OnRejected(uv_async_t *handle){
    DLOG_MINER(INFO) << "OnRejected received";

    auto miner = (BlockMiner*)handle->data;
    if(!miner->HasActiveProposal()){
      DLOG_MINER(ERROR) << "there is no active proposal";
      return;
    }

    //TODO:
    // - sanity check proposals for equivalency
    // - sanity check proposal state
    ProposalPtr proposal = miner->GetActiveProposal();
    DLOG_MINER(INFO) << "proposal " << proposal->raw() << " was rejected by peers.";


    DLOG_MINER_IF(ERROR, !miner->ClearActiveProposal()) << "cannot clear active proposal";
    DLOG_MINER_IF(ERROR, !miner->StartMiningTimer()) << "cannot start mining timer.";
  }

  void BlockMiner::OnMine(uv_timer_t* handle){
    auto miner = (BlockMiner*)handle->data;
    if(miner->HasActiveProposal()){
      DLOG_MINER(WARNING) << "mine called during active proposal.";
      return; // skip
    }

    ObjectPoolPtr pool = ObjectPool::GetInstance();
    if(pool->GetNumberOfUnsignedTransactions() == 0){
      DLOG_MINER(WARNING) << "skipping mining cycle, no transactions in pool.";
      return; // skip
    }

    // create a new block
    DLOG_MINER(INFO) << "creating new block....";
    BlockPtr blk = BlockBuilder::BuildNewBlock();
    DLOG_MINER(INFO) << "discovered new block: " << blk->hash();

    // create a new proposal
    Timestamp timestamp = Clock::now();
    UUID node_id = config::GetServerNodeID();
    ProposalPtr proposal = Proposal::NewInstance(uv_loop_new(), RawProposal(timestamp, UUID(), node_id, blk->GetHeader()));//TODO: can we leverage the block miner loop since it's in-active during proposals?
    if(!miner->SetActiveProposal(proposal)){
      LOG_MINER(ERROR) << "cannot set active proposal.";
      return;
    }

    auto job = new ProposalJob(proposal);
    DLOG_MINER_IF(ERROR, !Schedule(job)) << "cannot schedule new proposal job";
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
    return platform::ThreadJoin(thread_);
  }

  bool BlockMinerThread::Start(){
    return platform::ThreadStart(&thread_, BlockMiner::GetThreadName(), &HandleThread, (uword)BlockMiner::GetInstance());
  }

  void BlockMinerThread::HandleThread(uword param){
    if(!RegisterQueue()){
      LOG_MINER(ERROR) << "cannot register job queue.";
      return;
    }

    auto miner = (BlockMiner*)param;
    LOG_MINER_IF(ERROR, !miner->Run()) << "cannot run miner loop.";
  }
}