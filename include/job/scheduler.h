#ifndef TOKEN_SCHEDULER_H
#define TOKEN_SCHEDULER_H

#include <pthread.h>
#include "job/worker.h"

#include "utils/json_conversion.h"

namespace Token{
  class Job;
  class JobScheduler{
    friend class JobWorker;
   public:
    static const int32_t kMaxNumberOfJobs;
   private:
    JobScheduler() = delete;
   public:
    ~JobScheduler() = delete;

    static bool Initialize();
    static bool Schedule(Job* job);
    static JobWorker* GetWorker(const ThreadId& thread);
    static JobWorker* GetThreadWorker();
    static JobWorker* GetRandomWorker();
    static bool GetWorkerStatistics(JsonString& json);
  };
}

#endif //TOKEN_SCHEDULER_H