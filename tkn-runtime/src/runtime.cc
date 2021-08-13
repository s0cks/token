#include "common.h"
#include "runtime.h"

namespace token{
  Runtime::Runtime(uv_loop_t* loop):
    TestEventListener(loop),
    node_id_(true),
    config_(FLAGS_path + "/config"),
    events_(),
    loop_(loop),
    state_(State::kStopped),
    proposal_state_(this),
    task_queue_(task::kDefaultTaskEngineQueueSize),
    task_engine_(FLAGS_num_workers, 1),
    miner_(this),
    timer_(this),
    proposer_(this),
    acceptor_(this),
    pool_(FLAGS_path + "/pool"),
    server_(this){
    task_engine_.RegisterQueue(task_queue_);
    events_.Subscribe("on-test", &on_test_);
  }

  bool Runtime::Run(){
    //TODO: check state
    if(IsMiningEnabled() && !GetBlockMiner().StartTimer())
      return false;//TODO: better error handling
    if(IsServerEnabled() && !GetServer().Listen(FLAGS_server_port))
      return false;//TODO: better error handling
    proposal_state_.SubscribeTo(events_);
    VERIFY_UVRESULT2(FATAL, uv_run(loop(), UV_RUN_DEFAULT), "cannot run main loop");
    DLOG(INFO) << "the runtime has finished.";
    return true;
  }
}