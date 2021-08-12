#include "runtime.h"
#include "proposal.h"
#include "consensus/proposer.h"
#include "peer/peer_session_manager.h"

namespace token{
//  Proposer::ProposalPhaser::ProposalPhaser(Proposer* proposer):
//    proposer_(proposer),
//    poller_(),
//    timeout_(){
//    poller_.data = this;
//    CHECK_UVRESULT2(FATAL, uv_timer_init(proposer->GetRuntime()->loop(), &poller_), "cannot initialize poller timer");
//    timeout_.data = this;
//    CHECK_UVRESULT2(FATAL, uv_timer_init(proposer->GetRuntime()->loop(), &timeout_), "cannot initialize timeout timer");
//  }
//
//  void Proposer::ProposalPhaser::OnTimeout(uv_timer_t* handle){
//    auto phaser = (ProposalPhaser*)handle->data;
//    DLOG(INFO) << "the proposal has timed out.";
//    if(!phaser->TimeoutProposal())
//      DLOG(FATAL) << "cannot timeout proposal.";
//  }
//
//  void Proposer::ProposalPhaser::OnTick(uv_timer_t* handle){
//    auto phaser = (ProposalPhaser*)handle->data;
//    if(ProposalScope::HasRequiredVotes()){
//      DLOG(INFO) << "the proposal has the required votes.";
//      phaser->OnVotingFinished();
//      return;
//    }
//  }
//
//  Proposer::Phase1::Phase1(Proposer* proposer):
//    ProposalPhaser(proposer){}
//
//  bool Proposer::Phase1::Execute(){
//    ProposalScope::SetPhase(ProposalPhase::kPhase1);
//    PeerSessionManager::BroadcastPrepare();
//    ProposalScope::Promise(UUID());//TODO: get node_id
//    return StartPoller() && StartTimeout();
//  }
//
//  void Proposer::Phase1::OnVotingFinished(){
//    DLOG(INFO) << "proposal phase1 voting has finished.";
//    if(!StopPhaser())
//      DLOG(FATAL) << "cannot stop phase1 phaser.";
//
//    if(ProposalScope::WasPromised()){
//      if(!proposer()->phase2_.Execute())
//        DLOG(FATAL) << "cannot excecute proposal phase2.";
//      return;
//    } else if(ProposalScope::WasRejected()){
//      if(!RejectedProposal())
//        DLOG(FATAL) << "cannot reject proposal.";
//      return;
//    }
//
//    DLOG(FATAL) << "odd state managed w/ " << ProposalScope::GetCurrentVotes() << " votes.";
//  }
//
//  Proposer::Phase2::Phase2(Proposer* proposer):
//    ProposalPhaser(proposer){}
//
//  void Proposer::Phase2::OnVotingFinished(){
//    DLOG(INFO) << "proposal phase2 voting has finished.";
//    if(!StopPhaser())
//      DLOG(FATAL) << "cannot stop phase2 phaser.";
//
//    if(ProposalScope::WasAccepted()){
//      if(!AcceptedProposal())
//        DLOG(FATAL) << "cannot accept proposal.";
//      return;
//    } else if(ProposalScope::WasRejected()){
//      if(!RejectedProposal())
//        DLOG(FATAL) << "cannot reject proposal.";
//      return;
//    }
//
//    DLOG(FATAL) << "odd state managed w/ " << ProposalScope::GetCurrentVotes() << " votes.";
//  }
//
//  bool Proposer::Phase2::Execute(){
//    ProposalScope::SetPhase(ProposalPhase::kPhase2);
//    PeerSessionManager::BroadcastCommit();
//    ProposalScope::Accepted(UUID());//TODO: get node_id
//    return StartPoller() && StartTimeout();
////      auto& queue = proposer()->GetRuntime()->GetTaskQueue();
////      auto& engine = proposer()->GetRuntime()->GetTaskEngine();
////      atomic::LinkedList<leveldb::WriteBatch> write_queue;
////
////      auto start = Clock::now();
////      auto blk = Block::Genesis();//TODO: get proposal block
////      auto& transactions = blk->transactions();
////      for(auto& it : transactions){
////        auto task = new task::ProcessTransactionTask(&engine, write_queue, it);
////        if(!queue.Push(reinterpret_cast<uword>(task))){
////          LOG(FATAL) << "cannot submit new task to task queue.";
////          return false;
////        }
////
////        DLOG(INFO) << "waiting for task to finish.";
////        while(!task->IsFinished());//spin
////        DLOG(INFO) << "done waiting.";
////      }
////
////      DLOG(INFO) << "compiling batch.....";
////      DLOG(INFO) << "processing " << write_queue.size() << " writes....";
////      leveldb::WriteBatch batch;
////      while(!write_queue.empty()){
////        auto next = write_queue.pop_front();
////        DVLOG(2) << "appending batch of size " << next->ApproximateSize() << "b....";
////        batch.Append((*next));
////      }
////
////      auto end = Clock::now();
////      DLOG(INFO) << "batch compiled " << batch.ApproximateSize() << "b!";
////
////      auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
////      DLOG(INFO) << "processing took " << duration_ms.count() << "ms";
////
////      auto& pool = proposer()->GetRuntime()->GetPool();
////      leveldb::Status status;
////      if(!(status = pool.Write(batch)).ok()){
////        LOG(FATAL) << "cannot write batch to pool.";
////        return false;
////      }

  Proposer::Proposer(Runtime* runtime):
    ElectionEventListener(runtime->loop()),
    MinerEventListener(runtime->loop()),
    runtime_(runtime),
    prepare_(runtime, ProposalState::Phase::kPreparePhase),
    commit_(runtime, ProposalState::Phase::kCommitPhase){
    runtime_->AddMinerListener(this);
    runtime_->AddElectionListener(this);
  }

  void Proposer::HandleOnMine(){
    auto last_mined = GetRuntime()->GetBlockMiner().GetLastMined();
    auto& state = GetRuntime()->GetProposalState();
    if(state.IsActive()){
      DLOG(ERROR) << last_mined << " was mined, cannot create proposal there is already an active proposal.";
      return;
    }
    DVLOG(1) << last_mined << " was mined, creating proposal....";
    auto node_id = GetRuntime()->GetNodeId();
    if(!state.CreateNewProposal(node_id, last_mined))
      DLOG(FATAL) << "cannot create new proposal.";
    if(!StartProposal())
      DLOG(FATAL) << "cannot start new proposal.";
  }

  void Proposer::HandleOnElectionStart(){}
  void Proposer::HandleOnElectionPass(){}
  void Proposer::HandleOnElectionFail(){}

  void Proposer::HandleOnElectionTimeout(){
    auto& state = GetRuntime()->GetProposalState();
    auto& election = GetRuntime()->GetProposer().GetElectionForPhase(state.GetPhase());
    DVLOG(1) << "the election has timed out. (elapsed_ms=" << election.GetElectionElapsedTimeMilliseconds() << "ms, accepted=" << election.GetAcceptedPerc() << "%, rejected=" << election.GetRejectedPerc() << "%)";
  }

  void Proposer::HandleOnElectionFinished(){
    auto& state = GetRuntime()->GetProposalState();
    auto phase = state.GetPhase();
    auto& election = GetRuntime()->GetProposer().GetElectionForPhase(phase);
    DVLOG(1) << "the " << phase << " election has finished. (elapsed_ms=" << election.GetElectionElapsedTimeMilliseconds() << "ms, accepted=" << election.GetAcceptedPerc() << "%, rejected=" << election.GetRejectedPerc() << "%)";
    switch(phase){
      case ProposalState::Phase::kPreparePhase:{
        if(!commit_.StartElection()){
          DLOG(FATAL) << "cannot start election #2.";
          return;
        }
        return;
      }
      case ProposalState::Phase::kCommitPhase:{
        if(!EndProposal()){
          DLOG(FATAL) << "cannot finish the election.";
          return;
        }
        return;
      }
      default:
        DLOG(FATAL) << "unknown phase: " << phase;
        return;
    }
  }

  bool Proposer::StartProposal(){
    if(!GetRuntime()->OnProposalStart()){
      DLOG(ERROR) << "cannot call the OnProposalStart callback";
      return false;
    }
    if(!prepare_.StartElection()){
      DLOG(ERROR) << "cannot start election #1.";
      return false;
    }
    return true;
  }

  bool Proposer::EndProposal(){
    if(!GetRuntime()->OnProposalFinished()){
      DLOG(ERROR) << "cannot call OnProposalFinished callback";
      return false;
    }
    if(!GetRuntime()->GetProposalState().Clear()){
      DLOG(ERROR) << "cannot clear the current proposal.";
      return false;
    }
    return true;
  }
}