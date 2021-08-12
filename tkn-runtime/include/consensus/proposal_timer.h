#ifndef TKN_PROPOSAL_TIMER_H
#define TKN_PROPOSAL_TIMER_H

#include <uv.h>

namespace token{
  class Runtime;
  class ProposalEventListener;
  class ProposalTimer{
  protected:
    Runtime* runtime_;
    uv_timer_t handle_;

    void HandleOnTimeout();
  public:
    explicit ProposalTimer(Runtime* runtime);
    ~ProposalTimer() = default;

    bool Start();
    bool Stop();
  };
}

#endif//TKN_PROPOSAL_TIMER_H