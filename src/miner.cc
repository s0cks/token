#include <mutex>
#include <condition_variable>

#include "pool.h"
#include "miner.h"
#include "server/server.h"
#include "block_builder.h"
#include "proposal_handler.h"
#include "peer/peer_session_manager.h"
#include "consensus/proposal_manager.h"

namespace Token{
  static std::mutex mutex_;
  static std::condition_variable cond_;

  static ThreadId thread_;
  static BlockMiner::State state_ = BlockMiner::kStoppedState;
  static BlockMiner::Status status_ = BlockMiner::kOkStatus;
  static JobQueue queue_(JobScheduler::kMaxNumberOfJobs);

  static uv_async_t shutdown_;
  static uv_timer_t miner_timer_;

  BlockMiner::State BlockMiner::GetState(){
    std::lock_guard<std::mutex> guard(mutex_);
    return state_;
  }

  void BlockMiner::SetState(const State& state){
    std::unique_lock<std::mutex> lock(mutex_);
    state_ = state;
    lock.unlock();
    cond_.notify_all();
  }

  BlockMiner::Status BlockMiner::GetStatus(){
    std::lock_guard<std::mutex> guard(mutex_);
    return status_;
  }

  void BlockMiner::SetStatus(const Status& status){
    std::unique_lock<std::mutex> lock(mutex_);
    status_ = status;
    lock.unlock();
    cond_.notify_all();
  }

  int BlockMiner::StartMinerTimer(){
    return uv_timer_start(&miner_timer_, &HandleMine, FLAGS_miner_interval, FLAGS_miner_interval);
  }

  int BlockMiner::StopMinerTimer(){
    return uv_timer_stop(&miner_timer_);
  }

  void BlockMiner::HandleMine(uv_timer_t* handle){
    if(ProposalManager::HasProposal()){
      LOG(WARNING) << "mine called during active proposal.";
      return;
    }

    if(ObjectPool::GetNumberOfTransactions() == 0 || ObjectPool::GetNumberOfTransactions() < Block::kMaxTransactionsForBlock)
      return; // skip

    BlockPtr blk = BlockBuilder::BuildNewBlock();
    Hash hash = blk->GetHash();

    LOG(INFO) << "discovered block " << hash << ", creating proposal....";
#ifdef TOKEN_ENABLE_SERVER
    ProposalPtr proposal = std::make_shared<Proposal>(blk, Server::GetID());
#else
    ProposalPtr proposal = std::make_shared<Proposal>(blk, UUID());
#endif//TOKEN_ENABLE_SERVER

    if(!ProposalManager::SetProposal(proposal)){
      LOG(WARNING) << "cannot set active proposal to: " << proposal->ToString() << ", abandoning proposal.";
      return;
    }

    PeerSessionManager::BroadcastDiscoveredBlock();

    NewProposalHandler handler(queue_, proposal);
    if(!handler.ProcessProposal()){
      LOG(WARNING) << "couldn't process proposal #" << proposal->GetHeight() << ", abandoning proposal.";
      return;
    }
  }

  void BlockMiner::HandleThread(uword param){
    LOG(INFO) << "starting block miner thread....";
    SetState(BlockMiner::kStartingState);

    uv_loop_t* loop = uv_loop_new();
    uv_timer_init(loop, &miner_timer_);

    int err;
    if((err = StartMinerTimer()) != 0){
      LOG(WARNING) << "couldn't start the miner timer: " << uv_strerror(err);
      goto exit;
    }

    if((err = uv_run(loop, UV_RUN_DEFAULT)) != 0){
      LOG(WARNING) << "couldn't run miner loop: " << uv_strerror(err);
      goto exit;
    }
  exit:
    SetState(BlockMiner::kStoppedState);
    pthread_exit(0);
  }

  bool BlockMiner::Start(){
    if(!IsStoppedState())
      return false;

    if(!JobScheduler::RegisterQueue(thread_, &queue_)){
      LOG(WARNING) << "couldn't register block miner work queue.";
      return false;
    }

    return Thread::StartThread(&thread_, "miner", &HandleThread, 0);
  }

  bool BlockMiner::Stop(){
    if(!IsRunningState())
      return true; // should we return false?
    uv_async_send(&shutdown_);
    return Thread::StopThread(thread_);
  }
}