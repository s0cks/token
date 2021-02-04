#include <chrono>
#include "job/worker.h"
#include "job/scheduler.h"
#include "utils/timeline.h"

namespace token{
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

    LOG(INFO) << "starting worker....";
    instance->SetState(JobWorker::kRunning);

    char truncated_name[16];
    snprintf(truncated_name, 15, "worker-%" PRId16, instance->GetWorkerID());

    int result;
    if((result = pthread_setname_np(pthread_self(), truncated_name)) != 0){
      LOG(WARNING) << "couldn't set thread name: " << strerror(result);
      goto finish;
    }

    sleep(2);
    while(instance->IsRunning()){
      Job* next = instance->GetNextJob();
      if(next){
        LOG(INFO) << "[worker-" << instance->GetWorkerID() << "] running " << next->GetName() << "....";
        Counter& num_ran = instance->GetJobsRan();
        Histogram& histogram = instance->GetHistogram();
        Timestamp start = Clock::now();

        if(!next->Run()){
          LOG(WARNING) << "couldn't run the \"" << next->GetName() << "\" job.";
          continue;
        }

        num_ran->Increment();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);
        histogram->Update(duration_ms.count());
        LOG(INFO) << "[worker-" << instance->GetWorkerID() << "] " << next->GetName() << " has finished (" << duration_ms.count() << "ms).";
      }
    }
  finish:
    pthread_exit(nullptr);
  }
}