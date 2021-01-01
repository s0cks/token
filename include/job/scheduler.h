#ifndef TOKEN_SCHEDULER_H
#define TOKEN_SCHEDULER_H

#include <pthread.h>
#include "job/worker.h"

namespace Token{
  class Job;
  class JobScheduler{
    friend class JobWorker;
   public:
    static const int32_t kMaxNumberOfJobs;
    static const int16_t kMaxNumberOfWorkers;
   private:
    JobScheduler() = delete;
   public:
    ~JobScheduler() = delete;

    static bool Initialize();
    static bool Schedule(Job *job);
    static JobWorker *GetWorker(const std::thread::id &thread);
    static JobWorker *GetThreadWorker();
    static JobWorker *GetRandomWorker();
    static void PrintWorkerStatistics();
  };
}

#endif //TOKEN_SCHEDULER_H