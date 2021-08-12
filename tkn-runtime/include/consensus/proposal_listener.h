#ifndef TKN_PROPOSAL_LISTENER_H
#define TKN_PROPOSAL_LISTENER_H

#include <uv.h>
#include "common.h"

namespace token{
#define FOR_EACH_PROPOSAL_EVENT(V) \
  V(Start, on_start_) \
  V(Prepare, on_prepare_)          \
  V(Commit, on_commit_)            \
  V(Accepted, on_accepted_)        \
  V(Rejected, on_rejected_)        \
  V(Failed, on_failed_)            \
  V(Timeout, on_timeout_)          \
  V(Finished, on_finished_)

  class ProposalEventListener{
  protected:
    uv_async_t on_start_;
    uv_async_t on_prepare_;
    uv_async_t on_commit_;
    uv_async_t on_accepted_;
    uv_async_t on_rejected_;
    uv_async_t on_failed_;
    uv_async_t on_timeout_;
    uv_async_t on_finished_;

#define DECLARE_PROPOSAL_EVENT_HANDLER(Name, Handle) \
    virtual void HandleOnProposal##Name() = 0;
    FOR_EACH_PROPOSAL_EVENT(DECLARE_PROPOSAL_EVENT_HANDLER)
#undef DECLARE_PROPOSAL_EVENT_HANDLER
  public:
    explicit ProposalEventListener(uv_loop_t* loop):
      on_start_(),
      on_prepare_(),
      on_commit_(),
      on_accepted_(),
      on_rejected_(),
      on_failed_(),
      on_timeout_(),
      on_finished_(){
#define INITIALIZE_HANDLE(Name, Handle) \
      (Handle).data = this;             \
      CHECK_UVRESULT2(FATAL, uv_async_init(loop, &(Handle), [](uv_async_t* handle){ \
        return ((ProposalEventListener*)handle->data)->HandleOnProposal##Name(); \
      }), "cannot initialize the callback handle");
      FOR_EACH_PROPOSAL_EVENT(INITIALIZE_HANDLE)
#undef INITIALIZE_HANDLE
    }
    virtual ~ProposalEventListener() = default;

#define DEFINE_INVOKER(Name, Handle) \
    bool OnProposal##Name(){                 \
      VERIFY_UVRESULT2(FATAL, uv_async_send(&(Handle)), "cannot invoke the callback handle"); \
      return true;                   \
    }
    FOR_EACH_PROPOSAL_EVENT(DEFINE_INVOKER)
#undef DEFINE_INVOKER
  };

#define DECLARE_PROPOSAL_EVENT_HANDLER(Name, Handle) \
  void HandleOnProposal##Name() override;

#define DEFINE_PROPOSAL_EVENT_LISTENER \
  FOR_EACH_PROPOSAL_EVENT(DECLARE_PROPOSAL_EVENT_HANDLER)
}

#endif//TKN_PROPOSAL_LISTENER_H