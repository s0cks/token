#include <chrono>
#include "job/worker.h"
#include "job/scheduler.h"
#include "utils/timeline.h"

namespace token{
  static inline std::string
  GetWorkerName(const WorkerId& worker){
    char truncated_name[16];
    snprintf(truncated_name, 15, "worker-%" PRId16, worker);
    return std::string(truncated_name, 16);
  }


  Job* JobWorker::GetNextJob(){
    Job* job = queue_.Pop();
    if(job)
      return job;

    JobQueue* queue = JobScheduler::GetRandomQueue();
    if(!queue){
      pthread_yield();
      return nullptr;
    }

// TODO:
//    if(pthread_equal(worker->GetThreadID(), pthread_self())){
//      pthread_yield();
//      return nullptr;
//    }
    return queue->Steal();
  }

  void JobWorker::HandleThread(uword parameter){
    JobWorker* instance = ((JobWorker*) parameter);
#ifdef TOKEN_DEBUG
    THREAD_LOG(INFO) << "starting....";
#endif//TOKEN_DEBUG
    instance->SetState(JobWorker::kRunning);

    sleep(2);//TODO: remove this?
    while(instance->IsRunning()){
      Job* next = instance->GetNextJob();
      if(next){
        std::string job_name = next->GetName();

#ifdef TOKEN_DEBUG
        THREAD_LOG(INFO) << "running " << job_name << "....";
#endif//TOKEN_DEBUG
        //-----
        //TODO: refactor
        Counter& num_ran = instance->GetJobsRan();
        Histogram& histogram = instance->GetHistogram();
        Timestamp start = Clock::now();
        //-----

        if(!next->Run()){
          THREAD_LOG(WARNING) << "couldn't run the " << job_name << " job.";
          continue;
        }

        //---------
        //TODO: refactor
        num_ran->Increment();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
        histogram->Update(duration_ms.count());
        //---------

#ifdef TOKEN_DEBUG
        THREAD_LOG(INFO) << job_name << " has finished (" << duration_ms.count() << "ms).";
#endif//TOKEN_DEBUG
      }
    }

    instance->SetState(JobWorker::kStopping);
#ifdef TOKEN_DEBUG
    THREAD_LOG(INFO) << " stopped.";
#endif//TOKEN_DEBUG
    instance->SetState(JobWorker::kStopped);
    pthread_exit(nullptr);
  }
}