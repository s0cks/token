#include "miner.h"
#include "runtime.h"
#include "block_builder.h"

namespace token{
  BlockMiner::BlockMiner(Runtime* runtime):
    runtime_(runtime),
    timer_(),
    state_(State::kStoppedState),
    timeout_(0),
    repeat_(0){
    timer_.data = this;
    CHECK_UVRESULT2(FATAL, uv_timer_init(runtime->loop(), &timer_), "cannot initialize timer handle");
  }

  static inline void
  OnMine(uv_timer_t* handle){
    DLOG(INFO) << "OnMine.";
    auto miner = (BlockMiner*)handle->data;
    miner->PauseTimer();

    GenesisBlockBuilder builder;
    auto blk = builder.Build();

    if(!miner->GetRuntime()->StartProposal()){
      LOG(FATAL) << "couldn't start proposal";
      return;//TODO: better error handling
    }

    //TODO: resume mining
  }

  bool BlockMiner::StartTimer(){
    VERIFY_UVRESULT2(ERROR, uv_timer_start(&timer_, &OnMine, timeout_, repeat_), "cannot start the timer");
    DLOG(INFO) << "miner timer started.";
    return true;
  }

  bool BlockMiner::PauseTimer(){
    VERIFY_UVRESULT2(ERROR, uv_timer_stop(&timer_), "cannot stop the timer");
    DLOG(INFO) << "miner timer paused.";
    return true;
  }
}