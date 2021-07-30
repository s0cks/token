#include "miner.h"
#include "proposer.h"
#include "proposal_scope.h"
#include "peer/peer_session_manager.h"

#include "pool.h"
#include "runtime.h"
#include "tasks/task_process_transaction.h"

namespace token{
  Proposer::ProposalPhaser::ProposalPhaser(Proposer* proposer):
    proposer_(proposer),
    poller_(),
    timeout_(){
    poller_.data = this;
    CHECK_UVRESULT2(FATAL, uv_timer_init(proposer->GetRuntime()->loop(), &poller_), "cannot initialize poller timer");
    timeout_.data = this;
    CHECK_UVRESULT2(FATAL, uv_timer_init(proposer->GetRuntime()->loop(), &timeout_), "cannot initialize timeout timer");
  }

  void Proposer::ProposalPhaser::OnTimeout(uv_timer_t* handle){
    auto phaser = (ProposalPhaser*)handle->data;
    DLOG(INFO) << "the proposal has timed out.";
    if(!phaser->TimeoutProposal())
      DLOG(FATAL) << "cannot timeout proposal.";
  }

  void Proposer::ProposalPhaser::OnTick(uv_timer_t* handle){
    auto phaser = (ProposalPhaser*)handle->data;
    if(ProposalScope::HasRequiredVotes()){
      DLOG(INFO) << "the proposal has the required votes.";
      phaser->OnVotingFinished();
      return;
    }
  }

  Proposer::Phase1::Phase1(Proposer* proposer):
    ProposalPhaser(proposer){}

  bool Proposer::Phase1::Execute(){
    ProposalScope::SetPhase(ProposalPhase::kPhase1);
    PeerSessionManager::BroadcastPrepare();
    ProposalScope::Promise(UUID());//TODO: get node_id
    return StartPoller() && StartTimeout();
  }

  void Proposer::Phase1::OnVotingFinished(){
    DLOG(INFO) << "proposal phase1 voting has finished.";
    if(!StopPhaser())
      DLOG(FATAL) << "cannot stop phase1 phaser.";

    if(ProposalScope::WasPromised()){
      if(!proposer()->phase2_.Execute())
        DLOG(FATAL) << "cannot excecute proposal phase2.";
      return;
    } else if(ProposalScope::WasRejected()){
      if(!RejectedProposal())
        DLOG(FATAL) << "cannot reject proposal.";
      return;
    }

    DLOG(FATAL) << "odd state managed w/ " << ProposalScope::GetCurrentVotes() << " votes.";
  }

  Proposer::Phase2::Phase2(Proposer* proposer):
    ProposalPhaser(proposer){}

  void Proposer::Phase2::OnVotingFinished(){
    DLOG(INFO) << "proposal phase2 voting has finished.";
    if(!StopPhaser())
      DLOG(FATAL) << "cannot stop phase2 phaser.";

    if(ProposalScope::WasAccepted()){
      if(!AcceptedProposal())
        DLOG(FATAL) << "cannot accept proposal.";
      return;
    } else if(ProposalScope::WasRejected()){
      if(!RejectedProposal())
        DLOG(FATAL) << "cannot reject proposal.";
      return;
    }

    DLOG(FATAL) << "odd state managed w/ " << ProposalScope::GetCurrentVotes() << " votes.";
  }

  bool Proposer::Phase2::Execute(){
    ProposalScope::SetPhase(ProposalPhase::kPhase2);
    PeerSessionManager::BroadcastCommit();
    ProposalScope::Accepted(UUID());//TODO: get node_id
    return StartPoller() && StartTimeout();
//      auto& queue = proposer()->GetRuntime()->GetTaskQueue();
//      auto& engine = proposer()->GetRuntime()->GetTaskEngine();
//      atomic::LinkedList<leveldb::WriteBatch> write_queue;
//
//      auto start = Clock::now();
//      auto blk = Block::Genesis();//TODO: get proposal block
//      auto& transactions = blk->transactions();
//      for(auto& it : transactions){
//        auto task = new task::ProcessTransactionTask(&engine, write_queue, it);
//        if(!queue.Push(reinterpret_cast<uword>(task))){
//          LOG(FATAL) << "cannot submit new task to task queue.";
//          return false;
//        }
//
//        DLOG(INFO) << "waiting for task to finish.";
//        while(!task->IsFinished());//spin
//        DLOG(INFO) << "done waiting.";
//      }
//
//      DLOG(INFO) << "compiling batch.....";
//      DLOG(INFO) << "processing " << write_queue.size() << " writes....";
//      leveldb::WriteBatch batch;
//      while(!write_queue.empty()){
//        auto next = write_queue.pop_front();
//        DVLOG(2) << "appending batch of size " << next->ApproximateSize() << "b....";
//        batch.Append((*next));
//      }
//
//      auto end = Clock::now();
//      DLOG(INFO) << "batch compiled " << batch.ApproximateSize() << "b!";
//
//      auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//      DLOG(INFO) << "processing took " << duration_ms.count() << "ms";
//
//      auto& pool = proposer()->GetRuntime()->GetPool();
//      leveldb::Status status;
//      if(!(status = pool.Write(batch)).ok()){
//        LOG(FATAL) << "cannot write batch to pool.";
//        return false;
//      }
  }

  Proposer::Proposer(Runtime* runtime):
    runtime_(runtime),
    on_start_(),
    phase1_(this),
    phase2_(this),
    on_accepted_(),
    on_rejected_(),
    on_timeout_(){

    on_start_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_start_, &HandleOnStart), "cannot initialize the on_start_ callback");
    on_accepted_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_accepted_, &HandleOnAccepted), "cannot initialize the on_accepted_ callback");
    on_rejected_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_rejected_, &HandleOnRejected), "cannot initialize the on_rejected_ callback");
    on_timeout_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_timeout_, &HandleOnTimeout), "cannot initialize the on_timeout_ callback");
  }

  void Proposer::HandleOnStart(uv_async_t* handle){ //TODO: better error handling & logging
    auto proposer = (Proposer*)handle->data;
    auto height = 100;
    auto hash = Hash();
    auto proposal = ProposalScope::CreateNewProposal(height, hash);
    DLOG(INFO) << "starting proposal " << proposal->ToString() << "....";
    if(!proposer->phase1_.Execute())
      DLOG(FATAL) << "cannot execute proposal phase1.";
  }

  void Proposer::HandleOnAccepted(uv_async_t* handle){
    auto proposer = (Proposer*)handle->data;
    DLOG(FATAL) << "proposal was accepted by " << ProposalScope::GetRequiredVotes() << " peers.";
  }

  void Proposer::HandleOnRejected(uv_async_t* handle){
    //TODO: cleanup logging
    DLOG(FATAL) << "proposal was rejected by " << ProposalScope::GetRequiredVotes() << " peers.";
  }

  void Proposer::HandleOnTimeout(uv_async_t* handle){
    DLOG(FATAL) << "proposal timed out.";
  }
}