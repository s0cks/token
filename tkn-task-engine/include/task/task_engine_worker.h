#ifndef TKN_TASK_ENGINE_WORKER_H
#define TKN_TASK_ENGINE_WORKER_H

#include "platform.h"
#include "os_thread.h"
#include "task/task.h"
#include "task/task_queue.h"

namespace token{
  namespace task{
    typedef int WorkerId;

#define FOR_EACH_TASK_ENGINE_WORKER_STATE(V) \
    V(Starting)                              \
    V(Idle)                                  \
    V(Executing)                             \
    V(Stopping)                              \
    V(Stopped)

    class TaskEngine;

    class TaskEngineWorker{
      friend class TaskEngine;
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
      TaskEngine* engine_;
      ThreadId thread_;
      WorkerId worker_;
      atomic::RelaxedAtomic<State> state_;
      TaskQueue queue_;

      TaskEngineWorker(TaskEngine* engine, const WorkerId& worker, const int64_t& max_queue_size):
        engine_(engine),
        thread_(),
        worker_(worker),
        state_(State::kStopped),
        queue_(max_queue_size){}

      void SetState(const State& state){
        state_ = state;
      }

      Task* GetNextTask();
      static void HandleThread(uword parameter);
     public:
      TaskEngineWorker(const TaskEngineWorker& other) = delete;
      ~TaskEngineWorker() = default;

      TaskEngine* GetEngine() const{
        return engine_;
      }

      ThreadId GetThreadId() const{
        return thread_;
      }

      WorkerId& GetWorkerId(){
        return worker_;
      }

      WorkerId GetWorkerId() const{
        return worker_;
      }

      TaskQueue& GetTaskQueue(){
        return queue_;
      }

      State GetState() const{
        return (State)state_;
      }

#define DEFINE_CHECK(Name) \
      bool Is##Name() const{ return GetState() == State::k##Name; }
      FOR_EACH_TASK_ENGINE_WORKER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

      bool Start(){
        char thread_name[kThreadNameMaxLength];
        snprintf(thread_name, kThreadNameMaxLength, "worker-%" PRId16, GetWorkerId());
        return platform::ThreadStart(&thread_, thread_name, &HandleThread, reinterpret_cast<uword>(this));
      }

      bool Shutdown() const{
        return platform::ThreadJoin(thread_);
      }

      TaskEngineWorker& operator=(const TaskEngineWorker& rhs) = delete;
    };
  }
}

#endif//TKN_TASK_ENGINE_WORKER_H