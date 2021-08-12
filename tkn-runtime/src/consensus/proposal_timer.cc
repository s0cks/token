#include "runtime.h"
#include "consensus/proposal_timer.h"

namespace token{
  ProposalTimer::ProposalTimer(Runtime* runtime):
    runtime_(runtime),
    handle_(){
    handle_.data = this;
    CHECK_UVRESULT2(FATAL, uv_timer_init(runtime->loop(), &handle_), "cannot initialize the proposal timer handle");
  }

  bool ProposalTimer::Start(){
    auto callback = [](uv_timer_t* handle){
      return ((ProposalTimer*)handle->data)->HandleOnTimeout();
    };
    VERIFY_UVRESULT2(ERROR, uv_timer_start(&handle_, callback, FLAGS_proposal_timeout, 0), "cannot start the proposal timer handle");
    return true;
  }

  bool ProposalTimer::Stop(){
    VERIFY_UVRESULT2(ERROR, uv_timer_stop(&handle_), "cannot stop the proposal timer handle");
    return true;
  }

  void ProposalTimer::HandleOnTimeout(){
    DLOG(WARNING) << "the proposal has timed out.";
    if(!runtime_->OnProposalTimeout())
      DLOG(ERROR) << "cannot invoke the OnTimeout callback.";
  }
}