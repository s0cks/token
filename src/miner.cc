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
  static ThreadId thread_;
  static std::atomic<BlockMiner::State> state_ = { BlockMiner::kStoppedState };
  static std::atomic<BlockMiner::Status> status_ = { BlockMiner::kOkStatus };
  static JobQueue queue_(JobScheduler::kMaxNumberOfJobs);

  static uv_timer_t miner_timer_;
  static uv_async_t shutdown_;
  static uv_async_t on_promise_;
  static uv_async_t on_commit_;
  static uv_async_t on_quorum_;

  BlockMiner::State BlockMiner::GetState(){
    return state_.load(std::memory_order_relaxed);
  }

  void BlockMiner::SetState(const State& state){
    state_.store(state, std::memory_order_relaxed);
  }

  BlockMiner::Status BlockMiner::GetStatus(){
    return status_.load(std::memory_order_relaxed);
  }

  void BlockMiner::SetStatus(const Status& status){
    status_.store(status, std::memory_order_relaxed);
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

    uv_async_init(loop, &on_promise_, &OnPromiseCallback);
    uv_async_init(loop, &on_commit_, &OnCommitCallback);
    uv_async_init(loop, &on_quorum_, &OnQuorumCallback);

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

  void BlockMiner::OnPromise(){
    uv_async_send(&on_promise_);
  }

  void BlockMiner::OnCommit(){
    uv_async_send(&on_commit_);
  }

  void BlockMiner::OnQuorum(){
    uv_async_send(&on_quorum_);
  }

  void BlockMiner::OnPromiseCallback(uv_async_t* handle){
    ProposalPtr proposal = ProposalManager::GetProposal();
    PeerSession* session = proposal->GetPeer();
    if(!proposal->TransitionToPhase(Proposal::kVotingPhase))
      return session->SendRejected();
    return session->SendAccepted();
  }

  void BlockMiner::OnCommitCallback(uv_async_t* handle){
    ProposalPtr proposal = ProposalManager::GetProposal();
    PeerSession* session = proposal->GetPeer();
    if(!proposal->TransitionToPhase(Proposal::kCommitPhase))
      return session->SendRejected();
    return session->SendAccepted();
  }

  void BlockMiner::OnQuorumCallback(uv_async_t* handle){
    ProposalPtr proposal = ProposalManager::GetProposal();
    if(!proposal->TransitionToPhase(Proposal::kQuorumPhase))
      return;
  }
}