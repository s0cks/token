#include <chrono>
#include "job/worker.h"
#include "job/scheduler.h"
#include "utils/timeline.h"

namespace Token{
  Job* JobWorker::GetNextJob(){
    Job* job = queue_.Pop();
    if(job){
      return job;
    }

    JobWorker* worker = JobScheduler::GetRandomWorker();
    if(worker == this){
      pthread_yield();
      return nullptr;
    }

    if(!worker){
      pthread_yield();
      return nullptr;
    }
    return worker->queue_.Steal();
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
#ifdef TOKEN_DEBUG
        Counter& num_ran = instance->GetJobsRan();
        Histogram& histogram = instance->GetHistogram();

        Timepoint start = Clock::now();
#endif//TOKEN_DEBUG

        if(!next->Run()){
          LOG(WARNING) << "couldn't run the \"" << next->GetName() << "\" job.";
          continue;
        }

        num_ran->Increment();
#ifdef TOKEN_DEBUG
        LOG(INFO) << next->GetName() << " has finished.";
        auto total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
        histogram->Update(total_time_ms);
#endif//TOKEN_DEBUG
      }
    }
    finish:
    pthread_exit(nullptr);
  }
}