#ifndef TOKEN_MINER_H
#define TOKEN_MINER_H

#include <uv.h>
#include <mutex>
#include <ostream>
#include <glog/logging.h>

#include "flags.h"
#include "hash.h"
#include "common.h" //TODO: remove this include
#include "os_thread.h"
#include "atomic/relaxed_atomic.h"
#include "consensus/proposal_listener.h"

namespace token{
#define FOR_EACH_MINER_STATE(V) \
  V(Starting)                   \
  V(Running)                    \
  V(Stopping)                   \
  V(Stopped)

  class Runtime;
  class BlockMiner : public ProposalEventListener{
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_MINER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name##State: \
          return stream << #Name;
        FOR_EACH_MINER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
  private:
#ifdef TOKEN_DEBUG
    static const uint64_t kInitialDelayMilliseconds = 1000 * 5;
#else
    static const uint64_t kInitialDelayMilliseconds = 0;
#endif//TOKEN_DEBUG
   protected:
    Runtime* runtime_;
    uv_timer_t timer_;
    atomic::RelaxedAtomic<State> state_;

    std::mutex mtx_last_;
    Hash last_;

    void SetState(const State& state){
      state_ = state;
    }

    inline void
    SetLastMined(const Hash& hash){
      std::lock_guard<std::mutex> guard(mtx_last_);
      last_ = hash;
    }

    void HandleOnTick();

    DEFINE_PROPOSAL_EVENT_LISTENER;
   public:
    explicit BlockMiner(Runtime* runtime);
    ~BlockMiner() override = default;

    State GetState() const{
      return (State)state_;
    }

    Runtime* GetRuntime() const{
      return runtime_;
    }

    Hash GetLastMined(){
      std::lock_guard<std::mutex> guard(mtx_last_);
      return last_;
    }

    bool StartTimer();
    bool PauseTimer();

#define DEFINE_CHECK(Name) \
    inline bool Is##Name() const{ return (State)state_ == State::k##Name##State; }
    FOR_EACH_MINER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

    static inline const char*
    GetThreadName(){
      return "miner";
    }
  };
}

#endif//TOKEN_MINER_H