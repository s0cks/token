#ifndef TKN_PROPOSER_H
#define TKN_PROPOSER_H

#include <uv.h>
#include "proposal.h"

namespace token{
  class Runtime;
  class Proposer{
  private:
    Runtime* runtime_;
    uv_async_t on_start_;
    uv_async_t on_phase1_;
    uv_async_t on_phase2_;
    uv_async_t on_accepted_;
    uv_async_t on_rejected_;
    uv_async_t on_timeout_;

    static void HandleOnStart(uv_async_t*);
    static void HandleOnPhase1(uv_async_t*);
    static void HandleOnPhase2(uv_async_t*);
    static void HandleOnAccepted(uv_async_t*);
    static void HandleOnRejected(uv_async_t*);
    static void HandleOnTimeout(uv_async_t*);
  public:
    explicit Proposer(Runtime* runtime);
    ~Proposer() = default;

    Runtime* GetRuntime() const{
      return runtime_;
    }

    bool StartProposal(){//TODO: rename
      VERIFY_UVRESULT2(FATAL, uv_async_send(&on_start_), "cannot invoke on_start_proposal_ callback");
      return true;
    }

    bool ExecutePhase1(){
      VERIFY_UVRESULT2(FATAL, uv_async_send(&on_phase1_), "cannot invoke on_phase1_ callback");
      return true;
    }

    bool ExecutePhase2(){
      VERIFY_UVRESULT2(FATAL, uv_async_send(&on_phase2_), "cannot invoke on_phase2_ callback");
      return true;
    }

    bool OnAccepted(){
      VERIFY_UVRESULT2(FATAL, uv_async_send(&on_accepted_), "cannot invoke the on_accepted_ callback");
      return true;
    }

    bool OnRejected(){
      VERIFY_UVRESULT2(FATAL, uv_async_send(&on_rejected_), "cannot invoke the on_rejected_ callback");
      return true;
    }

    bool OnTimeout(){
      VERIFY_UVRESULT2(FATAL, uv_async_send(&on_timeout_), "cannot invoke on_timeout_ callback");
      return true;
    }
  };
}

#endif//TKN_PROPOSER_H