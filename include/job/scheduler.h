#ifndef TOKEN_SCHEDULER_H
#define TOKEN_SCHEDULER_H

#include "vthread.h"
#include "atomic/wsq.h"

namespace token{
  class Job;

  typedef WorkStealingQueue<Job*> JobQueue;

  class JobScheduler{
    friend class JobWorker;
    friend class BlockChain;
    friend class BlockMiner;
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

    #ifdef TOKEN_DEBUG
      static bool GetStats(Json::Writer& json);
    #endif//TOKEN_DEBUG
  };
}

#endif //TOKEN_SCHEDULER_H