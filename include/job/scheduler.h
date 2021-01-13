#ifndef TOKEN_SCHEDULER_H
#define TOKEN_SCHEDULER_H

#include <pthread.h>
#include "job/worker.h"

#include "utils/json_conversion.h"

namespace Token{
  class JobSchedulerStats{
   private:
    int64_t num_ran_;
    int64_t num_discarded_;
    JobWorkerStats* workers_;
   public:
    JobSchedulerStats():
      num_ran_(0),
      num_discarded_(0),
      workers_(new JobWorkerStats[FLAGS_num_workers]){}
    JobSchedulerStats(JobWorkerStats* stats):
      num_ran_(0),
      num_discarded_(0),
      workers_(new JobWorkerStats[FLAGS_num_workers]){
      for(int widx = 0; widx < FLAGS_num_workers; widx++){
        workers_[widx] = stats[widx];
        num_ran_ += stats[widx].GetNumberOfJobsRan();
        num_discarded_ += stats[widx].GetNumberOfJobsDiscarded();
      }
    }
    ~JobSchedulerStats() = default;

    int64_t GetNumberOfJobsRan() const{
      return num_ran_;
    }

    int64_t GetNumberOfJobsDiscarded() const{
      return num_discarded_;
    }

    JobWorkerStats& GetWorkerStats(int widx){
      return workers_[widx];
    }

    JobWorkerStats GetWorkerStats(int widx) const{
      return workers_[widx];
    }

    void operator=(const JobSchedulerStats& stats){
      num_ran_ = stats.num_ran_;
      num_discarded_ = stats.num_discarded_;
      for(int widx = 0; widx < FLAGS_num_workers; widx++)
        workers_[widx] = stats.workers_[widx];
    }
  };

  class Job;
  class JobScheduler{
    friend class JobWorker;
    friend class BlockChain;
    friend class BlockDiscoveryThread;
   public:
    static const int32_t kMaxNumberOfJobs;
   private:
    JobScheduler() = delete;

    static bool RegisterQueue(const ThreadId& thread, JobQueue* queue);
   public:
    ~JobScheduler() = delete;

    static bool Initialize();
    static bool Schedule(Job* job);
    static JobQueue* GetWorker(const ThreadId& thread);
    static JobQueue* GetThreadQueue();
    static JobQueue* GetRandomQueue();
    static JobWorker* GetRandomWorker();
    static JobSchedulerStats GetStats();
    static bool GetWorkerStatistics(JsonString& json);
  };
}

#endif //TOKEN_SCHEDULER_H