#ifndef TOKEN_WORKER_H
#define TOKEN_WORKER_H

#include <thread>
#include <atomic>
#include "job/job.h"
#include "job/queue.h"
#include "utils/metrics.h"

namespace Token{
  typedef int16_t JobWorkerId;

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

    friend std::ostream &operator<<(std::ostream &stream, const State &state){
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
    std::thread thread_;
    std::thread::id thread_id_;
    JobWorkerId id_;
    std::atomic<State> state_;
    JobQueue queue_;
    Histogram histogram_;
    Counter num_ran_;
    Counter num_discarded_;

    void SetState(const State &state){
      state_ = state;
    }

    bool Schedule(Job *job){
      return queue_.Push(job);
    }

    Histogram &GetHistogram(){
      return histogram_;
    }

    Job *GetNextJob();
    static void HandleThread(JobWorker *worker);
   public:
    JobWorker(const JobWorkerId &id, size_t max_queue_size):
      thread_(),
      thread_id_(),
      id_(id),
      state_(State::kStarting),
      queue_(max_queue_size),
      histogram_(new Metrics::Histogram("TaskHistogram")),
      num_ran_(new Metrics::Counter("NumRan")),
      num_discarded_(new Metrics::Counter("NumDiscarded")){}
    ~JobWorker() = default;

    JobWorkerId GetWorkerID() const{
      return id_;
    }

    std::thread::id GetThreadID() const{
      return thread_.get_id();
    }

    const std::atomic<State> &GetState(){
      return state_;
    }

    bool Wait(Job *job){
      while(!job->IsFinished());
      return true;
    }

    bool Start(){
      thread_ = std::thread(&HandleThread, this);
      thread_id_ = thread_.get_id();
      LOG(INFO) << "started worker: " << thread_id_;
      return true;
    }

    bool Stop(){
      SetState(JobWorker::kStopping);
      thread_.join();
      return true;
    }

    bool Submit(Job *job){
      return queue_.Push(job);
    }

#define DEFINE_CHECK(Name) \
        bool Is##Name(){ return state_ == State::k##Name; }
    FOR_EACH_JOB_POOL_WORKER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif //TOKEN_WORKER_H