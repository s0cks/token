#ifndef TOKEN_MINER_H
#define TOKEN_MINER_H

#include <ostream>
#include "vthread.h"
#include "consensus/proposal.h"

namespace token{
#define FOR_EACH_MINER_STATE(V) \
  V(Starting)                   \
  V(Running)                    \
  V(Stopping)                   \
  V(Stopped)

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
    uv_loop_t* loop_;
    RelaxedAtomic<State> state_;
    uv_timer_t timer_;
    ProposalPtr proposal_;

    void SetState(const State& state){
      state_ = state;
    }

    bool StopTimer();
    bool StartTimer();
    bool ClearCurrentProposal();
    ProposalPtr GetCurrentProposal();
    ProposalPtr CreateNewProposal();

    static void OnMine(uv_timer_t* handle);
   public:
    BlockMiner(uv_loop_t* loop=uv_loop_new()):
      loop_(loop),
      state_(State::kStoppedState),
      timer_(){
      timer_.data = this;
    }
    ~BlockMiner() = default;

    State GetState() const{
      return state_;
    }

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    bool Run();

#define DEFINE_CHECK(Name) \
    inline bool Is##Name() const{ return state_ == State::k##Name##State; }
    FOR_EACH_MINER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK
  };

  class BlockMinerThread{
   private:
    BlockMinerThread() = delete;

    static void HandleThread(uword param);
   public:
    ~BlockMinerThread() = delete;

    static bool Stop();
    static bool Start();
    static void Initialize();
  };
}

#endif//TOKEN_MINER_H