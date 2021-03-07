#ifndef TOKEN_WORKER_H
#define TOKEN_WORKER_H

#include <atomic>
#include "vthread.h"
#include "job/job.h"
#include "job/scheduler.h"

namespace token{
  typedef int32_t WorkerId;

  class JobWorkerStats{
   private:
    WorkerId worker_id_;
    int64_t num_ran_;
    int64_t num_discarded_;
    int64_t time_min_;
    int64_t time_mean_;
    int64_t time_max_;
   public:
    JobWorkerStats():
      worker_id_(0),
      num_ran_(0),
      num_discarded_(0),
      time_min_(0),
      time_mean_(0),
      time_max_(0){}
    JobWorkerStats(const WorkerId& worker_id, int64_t num_ran, int64_t num_discarded, int64_t time_min, int64_t time_mean, int64_t time_max):
      worker_id_(worker_id),
      num_ran_(num_ran),
      num_discarded_(num_discarded),
      time_min_(time_min),
      time_mean_(time_mean),
      time_max_(time_max){}
    JobWorkerStats(const JobWorkerStats& stats):
      worker_id_(stats.worker_id_),
      num_ran_(stats.num_ran_),
      num_discarded_(stats.num_discarded_),
      time_min_(stats.time_min_),
      time_mean_(stats.time_mean_),
      time_max_(stats.time_max_){}
    ~JobWorkerStats() = default;

    WorkerId GetWorkerID() const{
      return worker_id_;
    }

    int64_t GetNumberOfJobsRan() const{
      return num_ran_;
    }

    int64_t GetNumberOfJobsDiscarded() const{
      return num_discarded_;
    }

    int64_t GetMinimumTime() const{
      return time_min_;
    }

    int64_t GetMeanTime() const{
      return time_mean_;
    }

    int64_t GetMaximumTime() const{
      return time_max_;
    }

    void operator=(const JobWorkerStats& stats){
      worker_id_ = stats.worker_id_;
      num_ran_ = stats.num_ran_;
      num_discarded_ = stats.num_discarded_;
      time_min_ = stats.time_min_;
      time_mean_ = stats.time_mean_;
      time_max_ = stats.time_max_;
    }
  };

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

    void SetState(const State& state){
      state_.store(state, std::memory_order_seq_cst);
    }

    bool Schedule(Job* job){
      return queue_.Push(job);
    }

    JobQueue* GetQueue(){
      return &queue_;
    }

    Job* GetNextJob();
    static void HandleThread(uword parameter);
   public:
    JobWorker(const WorkerId& worker_id, int64_t max_queue_size):
      thread_(),
      worker_(worker_id),
      state_(State::kStarting),
      queue_(max_queue_size){}
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
      return ThreadStart(&thread_, name, &HandleThread, (uword) this);
    }

    bool Stop(){
      SetState(JobWorker::kStopping);
      return ThreadJoin(thread_);
    }

    bool Submit(Job* job){
      if(job != nullptr && !queue_.Push(job))
        return false;
      LOG(INFO) << "[worker-" << GetWorkerID() << "] submitting: " << job->GetName();
      return true;
    }

#define DEFINE_CHECK(Name) \
        bool Is##Name(){ return state_ == State::k##Name; }
    FOR_EACH_JOB_POOL_WORKER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif //TOKEN_WORKER_H