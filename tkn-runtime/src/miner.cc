#include "miner.h"
#include "runtime.h"
#include "block_builder.h"
#include "peer/peer_session_manager.h"

namespace token{
  BlockMiner::BlockMiner(Runtime* runtime):
    runtime_(runtime),
    timer_(),
    last_(),
    state_(State::kStoppedState){
    timer_.data = this;
    CHECK_UVRESULT2(FATAL, uv_timer_init(runtime->loop(), &timer_), "cannot initialize timer handle");
  }

  void BlockMiner::OnTick(uv_timer_t* handle){
    DLOG(INFO) << "OnMine.";
    auto miner = (BlockMiner*)handle->data;
    miner->PauseTimer();

    GenesisBlockBuilder builder;
    auto blk = builder.Build();
    auto hash = blk->hash();
    miner->SetLastMined(hash);

    if(!miner->GetRuntime()->StartProposal()){
      LOG(FATAL) << "couldn't start proposal";
      return;//TODO: better error handling
    }

    PeerSessionManager::BroadcastDiscovered();

    //TODO: resume mining
  }

  bool BlockMiner::StartTimer(){
    VERIFY_UVRESULT2(ERROR, uv_timer_start(&timer_, &OnTick, kInitialDelayMilliseconds, FLAGS_mining_interval), "cannot start the timer");
    DLOG(INFO) << "miner timer started.";
    return true;
  }

  bool BlockMiner::PauseTimer(){
    VERIFY_UVRESULT2(ERROR, uv_timer_stop(&timer_), "cannot stop the timer");
    DLOG(INFO) << "miner timer paused.";
    return true;
  }
}