#ifndef TOKEN_SCHEDULER_H
#define TOKEN_SCHEDULER_H

#include <pthread.h>
#include "job/worker.h"

namespace Token{
    class Job;
    class JobScheduler{
        friend class JobPoolWorker;
    public:
        static const int kMaxNumberOfJobs = 1024;
        static const int kMaxNumberOfWorkers = 10;
    private:
        JobScheduler() = delete;
    public:
        ~JobScheduler() = delete;

        static bool Initialize();
        static bool Schedule(Job* job);
        static JobPoolWorker* GetWorker(pthread_t wthread);
        static JobPoolWorker* GetThreadWorker();
        static JobPoolWorker* GetRandomWorker();
    };
}

#endif //TOKEN_SCHEDULER_H