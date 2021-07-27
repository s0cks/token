#include "common.h"
#include "runtime.h"

namespace token{
  Runtime::Runtime(uv_loop_t* loop):
    loop_(loop),
    state_(State::kStopped),
    task_queue_(task::kDefaultTaskEngineQueueSize),
    task_engine_(FLAGS_num_workers, 1),
    miner_(this),
    proposer_(this),
    pool_(FLAGS_path + "/pool"){
    task_engine_.RegisterQueue(task_queue_);
  }

  bool Runtime::Run(){
    //TODO: check state
    if(!StartBlockMiner())
      return false;//TODO: better error handling
    VERIFY_UVRESULT2(FATAL, uv_run(loop(), UV_RUN_DEFAULT), "cannot run main loop");
    return true;
  }
}