#ifndef TOKEN_WORKER_H
#define TOKEN_WORKER_H

#include <atomic>
#include "vthread.h"
#include "job/job.h"
#include "job/queue.h"
#include "utils/metrics.h"

namespace Token{
  typedef int8_t WorkerId;

#define FOR_EACH_JOB_POOL_WORKER_STATE(V) \
    V(Starting)                           \
    V(Idle)                               \
    V(Running)                            \
    V(Stopping)                           \
    V(Stopped)

  class JobWorker{
    friend class JobScheduler;
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_JOB_POOL_WORKER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_JOB_POOL_WORKER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }
   private:
    ThreadId thread_;
    WorkerId worker_;

    std::atomic<State> state_;
    JobQueue queue_;
    Histogram histogram_;
    Counter num_ran_;
    Counter num_discarded_;

    void SetState(const State& state){
      state_.store(state, std::memory_order_seq_cst);
    }

    bool Schedule(Job* job){
      return queue_.Push(job);
    }

    Histogram& GetHistogram(){
      return histogram_;
    }

    Counter& GetJobsRan(){
      return num_ran_;
    }

    Counter& GetJobsDiscarded(){
      return num_discarded_;
    }

    Job* GetNextJob();
    static void HandleThread(uword parameter);
   public:
    JobWorker(const WorkerId& worker_id, size_t max_queue_size):
      thread_(),
      worker_(worker_id),
      state_(State::kStarting),
      queue_(max_queue_size),
      histogram_(new Metrics::Histogram("TaskHistogram")),
      num_ran_(new Metrics::Counter("NumRan")),
      num_discarded_(new Metrics::Counter("NumDiscarded")){}
    ~JobWorker() = delete;

    ThreadId GetThreadID() const{
      return thread_;
    }

    WorkerId GetWorkerID() const{
      return worker_;
    }

    State GetState() const{
      return state_.load(std::memory_order_seq_cst);
    }

    bool Wait(Job* job){
      while(!job->IsFinished());
      return true;
    }

    bool Start(){
      char name[16];
      snprintf(name, 16, "worker-%" PRId16, GetWorkerID());
      return Thread::StartThread(&thread_, name, &HandleThread, (uword) this);
    }

    bool Stop(){
      SetState(JobWorker::kStopping);
      return Thread::StopThread(thread_);
    }

    bool Submit(Job* job){
      return queue_.Push(job);
    }

#define DEFINE_CHECK(Name) \
        bool Is##Name(){ return state_ == State::k##Name; }
    FOR_EACH_JOB_POOL_WORKER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif //TOKEN_WORKER_H