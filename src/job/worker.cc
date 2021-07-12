#include <chrono>
#include "job/worker.h"
#include "job/scheduler.h"

#include "timestamp.h"

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
    DLOG(INFO) << "starting....";
    instance->SetState(JobWorker::kRunning);

    sleep(2);//TODO: remove this?
    while(instance->IsRunning()){
      Job* next = instance->GetNextJob();
      if(next){
        std::string job_name = next->GetName();

        DLOG(INFO) << "running " << job_name << "....";
        Timestamp start = Clock::now();

        if(!next->Run()){
          DLOG(WARNING) << "couldn't run the " << job_name << " job.";
          continue;
        }

        //---------
        //TODO: refactor
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
        //---------

        DLOG(INFO) << job_name << " has finished (" << duration_ms.count() << "ms).";
      }
    }

    instance->SetState(JobWorker::kStopping);
    // do we need to perform some shutdown logic?

    instance->SetState(JobWorker::kStopped);
    DLOG(INFO) << "stopped.";
    pthread_exit(nullptr);
  }
}