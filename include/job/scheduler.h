#ifndef TOKEN_SCHEDULER_H
#define TOKEN_SCHEDULER_H

#include "../../tkn-platform/include/os_thread.h"
#include "atomic/work_stealing_queue.h"

namespace token{
  class Job;

  typedef WorkStealingQueue<Job*> JobQueue;

  class JobScheduler{
    friend class JobWorker;
    friend class BlockChain;
    friend class BlockMiner;
   public:
    static const int32_t kMaxNumberOfJobs;
   private:
    JobScheduler() = delete;
   public:
    ~JobScheduler() = delete;

    static bool Initialize();
    static bool Schedule(Job* job);
    static bool RegisterQueue(const ThreadId& thread, JobQueue* queue);
    static JobQueue* GetWorker(const ThreadId& thread);
    static JobQueue* GetThreadQueue();
    static JobQueue* GetRandomQueue();

    static inline const char*
    GetName(){
      return "JobScheduler";
    }
  };
}

#endif //TOKEN_SCHEDULER_H