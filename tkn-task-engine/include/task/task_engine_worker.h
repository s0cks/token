#ifndef TKN_TASK_ENGINE_WORKER_H
#define TKN_TASK_ENGINE_WORKER_H

#include "os_thread.h"
#include "task/task.h"

namespace token{
  namespace task{
    typedef int WorkerId;

#define FOR_EACH_TASK_ENGINE_WORKER_STATE(V) \
    V(Starting)                              \
    V(Idle)                                  \
    V(Executing)                             \
    V(Stopping)                              \
    V(Stopped)

    class TaskEngineWorker{
     public:
      enum State{
#define DEFINE_STATE(Name) k##Name,
        FOR_EACH_TASK_ENGINE_WORKER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
      };

      friend std::ostream& operator<<(std::ostream& stream, const State& state) {
        switch(state){
#define DEFINE_TOSTRING(Name) \
          case State::k##Name:\
            return stream << #Name;
          FOR_EACH_TASK_ENGINE_WORKER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
          default:
            return stream << "Unknown";
        }
      }
     protected:
      ThreadId thread_;
      WorkerId worker_;
      RelaxedAtomic<State> state_;
      int64_t max_queue_size_;

      void SetState(const State& state){
        state_ = state;
      }

      static void HandleThread(uword parameter);
     public:
      TaskEngineWorker(const WorkerId& worker, const int64_t& max_queue_size):
        thread_(),
        worker_(worker),
        state_(State::kStopped),
        max_queue_size_(max_queue_size){}
      ~TaskEngineWorker() = default;

      ThreadId GetThreadId() const{
        return thread_;
      }

      WorkerId GetWorkerId() const{
        return worker_;
      }

      State GetState() const{
        return (State)state_;
      }

#define DEFINE_CHECK(Name) \
      bool Is##Name() const{ return GetState() == State::k##Name; }
      FOR_EACH_TASK_ENGINE_WORKER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

      int64_t GetMaxQueueSize() const{
        return max_queue_size_;
      }

      bool Start();
      bool Shutdown();
    };
  }
}

#endif//TKN_TASK_ENGINE_WORKER_H