#ifndef TOKEN_MINER_H
#define TOKEN_MINER_H

#include <uv.h>
#include <ostream>
#include <glog/logging.h>

#include "flags.h"
#include "common.h" //TODO: remove this include
#include "os_thread.h"
#include "atomic/relaxed_atomic.h"

namespace token{
#define FOR_EACH_MINER_STATE(V) \
  V(Starting)                   \
  V(Running)                    \
  V(Stopping)                   \
  V(Stopped)

  class Runtime;
  class BlockMiner{
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
   protected:
    Runtime* runtime_;
    uv_timer_t timer_;
    atomic::RelaxedAtomic<State> state_;
    int64_t timeout_;
    int64_t repeat_;

    void SetState(const State& state){
      state_ = state;
    }
   public:
    explicit BlockMiner(Runtime* runtime);
    ~BlockMiner() = default;

    State GetState() const{
      return (State)state_;
    }

    Runtime* GetRuntime() const{
      return runtime_;
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