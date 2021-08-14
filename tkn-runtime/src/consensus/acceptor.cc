#include "runtime.h"
#include "proposal.h"
#include "consensus/acceptor.h"
#include "tasks/task_process_transaction.h"

namespace token{
  Acceptor::Acceptor(Runtime* runtime):
    ProposalEventListener(runtime->loop(), runtime->GetEventBus()),
    ElectionEventListener(runtime->loop(), runtime->GetEventBus()),
    runtime_(runtime){}

  void Acceptor::HandleOnProposalStart(){
    auto& state = GetRuntime()->GetProposalState();
    DVLOG(1) << "proposal " << state.GetProposalId() << " has been started by " << state.GetProposerId();
  }

  void Acceptor::HandleOnProposalPrepare(){

  }

  void Acceptor::HandleOnProposalCommit(){}

  void Acceptor::HandleOnProposalAccepted(){
    auto& state = GetRuntime()->GetProposalState();
    auto proposal = state.GetProposal();
    auto node_id = GetRuntime()->GetNodeId();
  }

  void Acceptor::HandleOnProposalRejected(){
    auto& state = GetRuntime()->GetProposalState();
    auto proposal = state.GetProposal();
    auto node_id = GetRuntime()->GetNodeId();
  }

  void Acceptor::HandleOnProposalFailed(){
    auto& state = GetRuntime()->GetProposalState();
    auto proposal = state.GetProposal();
    auto node_id = GetRuntime()->GetNodeId();
  }

  void Acceptor::HandleOnProposalTimeout(){
    auto& state = GetRuntime()->GetProposalState();
    auto proposal = state.GetProposal();
    auto node_id = GetRuntime()->GetNodeId();
  }

  void Acceptor::HandleOnProposalFinished(){
    auto& state = GetRuntime()->GetProposalState();
    auto proposal = state.GetProposal();
    auto node_id = GetRuntime()->GetNodeId();
  }

  Election& Acceptor::GetCurrentElection(const ProposalState::Phase& phase) const{
    auto& proposer = GetRuntime()->GetProposer();
    switch(phase){
      case ProposalState::Phase::kPreparePhase:
        return proposer.GetPrepareElection();
      case ProposalState::Phase::kCommitPhase:
        return proposer.GetCommitElection();
      default:
        DLOG(FATAL) << "unknown proposal phase: " << phase;
    }
  }

  bool Acceptor::IsValidProposal(const UUID& proposal_id, const Hash& hash){
    DVLOG(1) << "validating proposal " << proposal_id << ".....";
    auto& pool = GetRuntime()->GetPool();
    if(!pool.HasBlock(hash)){
      DLOG(ERROR) << "cannot find block " << hash << " in pool.";
      return false;
    }
    //TODO: verify block
    return true;
  }

  bool Acceptor::CommitProposal(const UUID& proposal_id, const Hash& hash){
    auto& engine = GetRuntime()->GetTaskEngine();
    auto& queue = GetRuntime()->GetTaskQueue();
    auto& pool = GetRuntime()->GetPool();

    auto start = Clock::now();
    auto blk = Block::Genesis();//TODO: get proposal block
    IndexedTransactionSet transactions;
    blk->GetTransactions(transactions);
    for(auto& it : transactions){
      auto task = new task::ProcessTransactionTask(&engine, pool, it);
      if(!queue.Push(reinterpret_cast<uword>(task))){
        LOG(FATAL) << "cannot submit new task to task queue.";
        return false;
      }

      DLOG(INFO) << "waiting for task to finish.";
      while(!task->IsFinished());//spin
      DLOG(INFO) << "done waiting.";
      DLOG(INFO) << "processed " << task->GetNumberOfInputsProcessed() << " inputs.";
      DLOG(INFO) << "processed " << task->GetNumberOfOutputsProcessed() << " outputs.";
    }

    auto end = Clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    DLOG(INFO) << "processing took " << duration_ms.count() << "ms";
    return true;
  }

  void Acceptor::AcceptProposal(const ProposalState::Phase& phase, const UUID& node_id){
    auto& state = GetRuntime()->GetProposalState();
    if(state.WasProposedBy(node_id)){
      GetCurrentElection(phase).Accept(node_id);
    } else{
      auto session = state.GetProposer();
      if(!session->SendAccepted())
        DLOG(FATAL) << "cannot SendAccepted()";
    }
  }

  void Acceptor::RejectProposal(const ProposalState::Phase& phase, const UUID& node_id){
    auto& state = GetRuntime()->GetProposalState();
    if(!state.WasProposedBy(node_id)){
      GetCurrentElection(phase).Reject(node_id);
    } else{
      auto session = state.GetProposer();
      if(!session->SendRejected())
        DLOG(FATAL) << "cannot SendRejected()";
    }
  }

  void Acceptor::HandleOnElectionStart(){//TODO: cleanup function
    auto& state = GetRuntime()->GetProposalState();
    auto node_id = GetRuntime()->GetNodeId();
    auto phase = state.GetPhase();
    auto proposal = state.GetProposal();
    auto proposal_id = proposal->proposal_id();
    auto hash = proposal->hash();

    switch(phase){
      case ProposalState::Phase::kPreparePhase:{
        if(IsValidProposal(proposal_id, hash)){
          DVLOG(1) << "proposal is valid, accepting....";
          AcceptProposal(phase, node_id);
          return;
        }
      }
      case ProposalState::Phase::kCommitPhase:{
        if(CommitProposal(proposal_id, hash)){
          DVLOG(1) << "proposal is committed, accepting....";
          AcceptProposal(phase, node_id);
          return;
        }
      }
      default:
        DLOG(FATAL) << "unknown proposal phase: " << phase;
        return;
    }

    DVLOG(1) << "proposal is not valid, rejecting....";
    RejectProposal(phase, node_id);
  }

  void Acceptor::HandleOnElectionPass(){}
  void Acceptor::HandleOnElectionFail(){}
  void Acceptor::HandleOnElectionTimeout(){}

  void Acceptor::HandleOnElectionFinished(){

  }
}