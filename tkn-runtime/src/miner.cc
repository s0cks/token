#include "pool.h"
#include "miner.h"
#include "runtime.h"
#include "block_builder.h"
#include "peer/peer_session_manager.h"

namespace token{
  BlockMiner::BlockMiner(Runtime* runtime):
      ProposalEventListener(runtime->loop()),
      runtime_(runtime),
      timer_(),
      last_(),
      state_(State::kStoppedState){
    runtime_->AddProposalListener(this);

    timer_.data = this;
    CHECK_UVRESULT2(FATAL, uv_timer_init(runtime->loop(), &timer_), "cannot initialize timer handle");
  }

  void BlockMiner::HandleOnTick(){
    GenesisBlockBuilder builder;
    auto blk = builder.Build();
    auto hash = blk->hash();
    SetLastMined(hash);

    auto& pool = GetRuntime()->GetPool();
    if(!pool.PutBlock(hash, blk)){
      DLOG(FATAL) << "cannot put block in pool.";
      return;
    }

    if(!GetRuntime()->OnMine()){
      DLOG(FATAL) << "cannot invoke OnMine callback";
      return;
    }
    //TODO: broadcast prepare
  }

  bool BlockMiner::StartTimer(){
    VERIFY_UVRESULT2(ERROR, uv_timer_start(&timer_, [](uv_timer_t* handle){
      return ((BlockMiner*)handle->data)->HandleOnTick();
    }, kInitialDelayMilliseconds, FLAGS_mining_interval), "cannot start the timer");
    return true;
  }

  bool BlockMiner::PauseTimer(){
    VERIFY_UVRESULT2(ERROR, uv_timer_stop(&timer_), "cannot stop the timer");
    return true;
  }

  void BlockMiner::HandleOnProposalStart(){
    auto& state = GetRuntime()->GetProposalState();
    auto proposal = state.GetProposal();
    DVLOG(1) << "proposal " << proposal->proposal_id() << " has started, pausing block miner....";
    if(!PauseTimer())
      DLOG(FATAL) << "cannot pause the miner.";
  }

  void BlockMiner::HandleOnProposalPrepare(){}
  void BlockMiner::HandleOnProposalCommit(){}
  void BlockMiner::HandleOnProposalAccepted(){}
  void BlockMiner::HandleOnProposalRejected(){}
  void BlockMiner::HandleOnProposalFailed(){}
  void BlockMiner::HandleOnProposalTimeout(){}

  void BlockMiner::HandleOnProposalFinished(){
    auto& state = GetRuntime()->GetProposalState();
    auto proposal = state.GetProposal();
    if(!StartTimer())
      DLOG(FATAL) << "cannot resume the miner.";
  }
}