#ifndef TOKEN_MINER_H
#define TOKEN_MINER_H

#include <ostream>
#include "vthread.h"

namespace Token{
#define FOR_EACH_MINER_STATE(V) \
  V(Starting)                   \
  V(Running)                    \
  V(Stopping)                   \
  V(Stopped)

#define FOR_EACH_MINER_STATUS(V) \
  V(Ok)                          \
  V(Warning)                     \
  V(Error)

  class BlockMiner : Thread{
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

    enum Status{
#define DEFINE_STATUS(Name) k##Name##Status,
      FOR_EACH_MINER_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
        case Status::k##Name##Status: \
          return stream << #Name;
        FOR_EACH_MINER_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   private:
    BlockMiner() = delete;

    static void SetState(const State& state);
    static void SetStatus(const Status& status);
    static void HandleMine(uv_timer_t* handle);

    static int StartMinerTimer();
    static int StopMinerTimer();

    static void HandleThread(uword param);
   public:
    ~BlockMiner() = delete;

    static State GetState();
    static Status GetStatus();
    static bool Stop();
    static bool Start();

#define DEFINE_CHECK(Name) \
    static inline bool Is##Name##State(){ return GetState() == State::k##Name##State; }
    FOR_EACH_MINER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

#define DEFINE_CHECK(Name) \
    static inline bool Is##Name##Status(){ return GetStatus() == Status::k##Name##Status; }
    FOR_EACH_MINER_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif//TOKEN_MINER_H