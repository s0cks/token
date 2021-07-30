#include "runtime.h"
#include "acceptor.h"

#include "proposal_scope.h"
#include "node/node_session.h"

namespace token{
  Acceptor::Acceptor(Runtime* runtime):
    runtime_(runtime),
    on_prepare_(),
    on_commit_(),
    on_accepted_(),
    on_rejected_(),
    timeout_(){

    on_prepare_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_prepare_, &HandleOnPrepare), "cannot initialize on_prepare_ callback");
    on_commit_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_commit_, &HandleOnCommit), "cannot initialize on_commit_ callback");
    on_accepted_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_accepted_, &HandleOnAccepted), "cannot initialize on_accepted callback");
    on_rejected_.data = this;
    CHECK_UVRESULT2(FATAL, uv_async_init(runtime->loop(), &on_rejected_, &HandleOnRejected), "cannot initialize uv_rejected callback");

    timeout_.data = this;
    CHECK_UVRESULT2(FATAL, uv_timer_init(runtime->loop(), &timeout_), "cannot initialize timeout_ timer handle");
  }

  void Acceptor::HandleOnPrepare(uv_async_t* handle){
    //TODO:
    // - check for block
    // - verify block

    auto acceptor = (Acceptor*)handle->data;
    if(!acceptor->StartTimeout())
      DLOG(FATAL) << "cannot start acceptor timeout.";
  }

  void Acceptor::HandleOnCommit(uv_async_t* handle){
    //TODO:
    // - commit block
    // - send accepted or rejected
    auto acceptor = (Acceptor*)handle->data;
    auto proposer = ProposalScope::GetProposer();

    bool valid = true;
    if(valid){
      proposer->SendAccepted();
      if(!acceptor->StopTimeout())
        DLOG(FATAL) << "cannot stop the acceptor timeout.";
    } else{
      proposer->SendRejected();
      if(!acceptor->StopTimeout())
        DLOG(FATAL) << "cannot stop the acceptor timeout.";
    }
  }

  void Acceptor::HandleOnAccepted(uv_async_t* handle){

  }

  void Acceptor::HandleOnRejected(uv_async_t* handle){
    //TODO: stop timeout
  }

  void Acceptor::HandleOnTimeout(uv_timer_t* handle){
    //TODO: bad things
    DLOG(FATAL) << "proposal timed out.";
  }
}