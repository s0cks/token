#ifndef TKN_PROPOSER_H
#define TKN_PROPOSER_H

#include <uv.h>
#include "proposal.h"

namespace token{
  class Runtime;
  class Proposer{
  private:
    class ProposalPhaser{
    public:
      static const uint64_t kPollerIntervalMilliseconds = 500;
#ifdef TOKEN_DEBUG
      static const uint64_t kTimeoutMilliseconds = 1000 * 30; // 30 seconds
#else
      static const uint64_t kTimeoutMilliseconds = 1000 * 60 * 60 * 10; // 10 minutes
#endif//TOKEN_DEBUG
    protected:
      Proposer* proposer_;
      uv_timer_t poller_;
      uv_timer_t timeout_;

      explicit ProposalPhaser(Proposer* proposer);
      virtual void OnVotingFinished() = 0;

      inline Proposer*
      proposer() const{
        return proposer_;
      }

      inline bool
      StartPoller(){
        VERIFY_UVRESULT2(FATAL, uv_timer_start(&poller_, &OnTick, 0, kPollerIntervalMilliseconds), "cannot start the poller timer handle");
        return true;
      }

      inline bool
      StopPoller(){
        VERIFY_UVRESULT2(FATAL, uv_timer_stop(&poller_), "cannot stop the poller timer handle");
        return true;
      }

      inline bool
      StartTimeout(){
        VERIFY_UVRESULT2(FATAL, uv_timer_start(&timeout_, &OnTimeout, kTimeoutMilliseconds, 0), "cannot start the timeout timer handle");
        return true;
      }

      inline bool
      StopTimeout(){
        VERIFY_UVRESULT2(FATAL, uv_timer_stop(&timeout_), "cannot stop the timeout timer handle");
        return true;
      }

      inline bool
      StopPhaser(){
        return StopTimeout() && StopPoller();
      }

      inline bool
      AcceptedProposal(){
        return StopPhaser() && proposer()->OnAccepted();
      }

      inline bool
      RejectedProposal(){
        return StopPhaser() && proposer()->OnRejected();
      }

      inline bool
      TimeoutProposal(){
        return StopTimeout() && proposer()->OnTimeout();
      }

      static void OnTick(uv_timer_t* handle);
      static void OnTimeout(uv_timer_t* handle);
    public:
      virtual ~ProposalPhaser() = default;

      virtual bool Execute() = 0;
    };

    class Phase1  : public ProposalPhaser{
    protected:
      void OnVotingFinished() override;
    public:
      explicit Phase1(Proposer* proposer);
      ~Phase1() override = default;
      bool Execute() override;
    };

    class Phase2 : public ProposalPhaser{
    protected:
      void OnVotingFinished() override;
    public:
      explicit Phase2(Proposer* proposer);
      ~Phase2() override = default;
      bool Execute() override;
    };

    Runtime* runtime_;
    uv_async_t on_start_;
    Phase1 phase1_;
    Phase2 phase2_;
    uv_async_t on_accepted_;
    uv_async_t on_rejected_;
    uv_async_t on_timeout_;

    static void HandleOnStart(uv_async_t*);
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