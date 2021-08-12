#include "common.h"
#include "runtime.h"

namespace token{
  Runtime::Runtime(uv_loop_t* loop):
    ProposalEventListener(loop),
    MinerEventListener(loop),
    ElectionEventListener(loop),
    node_id_(true),
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
  }

#define DEFINE_HANDLE_EVENT(Name, Handle) \
  void Runtime::HandleOnProposal##Name(){         \
    auto propagator = [](ProposalEventListener* listener){ \
      if(!listener->OnProposal##Name())         \
        DLOG(ERROR) << "cannot propagate the " << #Name << " event."; \
    };                                    \
    Propagate(proposal_listeners_, #Name, propagator);\
  }

  FOR_EACH_PROPOSAL_EVENT(DEFINE_HANDLE_EVENT)
#undef DEFINE_HANDLE_EVENT

#define DEFINE_HANDLE_MINER_EVENT(Name, Handle) \
  void Runtime::HandleOn##Name(){               \
    auto propagator = [](MinerEventListener* listener){ \
      if(!listener->On##Name())                 \
        DLOG(ERROR) << "cannot propagate the " << #Name << " callback."; \
    };                                          \
    Propagate(miner_listeners_, #Name, propagator);    \
  }

  FOR_EACH_MINER_EVENT(DEFINE_HANDLE_MINER_EVENT)
#undef DEFINE_HANDLE_MINER_EVENT

#define DEFINE_HANDLE_ELECTION_EVENT(Name, Handle) \
  void Runtime::HandleOnElection##Name(){          \
    auto fn = std::bind(&ElectionEventListener::OnElection##Name, std::placeholders::_1);                                               \
    std::for_each(election_listeners_.begin(), election_listeners_.end(), fn);        \
  }
  FOR_EACH_ELECTION_EVENT(DEFINE_HANDLE_ELECTION_EVENT)
#undef DEFINE_HANDLE_ELECTION_EVENT

  bool Runtime::Run(){
    //TODO: check state
    if(IsMiningEnabled() && !GetBlockMiner().StartTimer())
      return false;//TODO: better error handling
    if(IsServerEnabled() && !GetServer().Listen(FLAGS_server_port))
      return false;//TODO: better error handling
    VERIFY_UVRESULT2(FATAL, uv_run(loop(), UV_RUN_DEFAULT), "cannot run main loop");
    DLOG(INFO) << "the runtime has finished.";
    return true;
  }
}