#include "job/worker.h"
#include "job/scheduler.h"

namespace Token{
    Job* JobPoolWorker::GetNextJob(){
        Job* job = queue_.Pop();
        if(job){
            return job;
        } else{
            JobPoolWorker* worker = JobScheduler::GetRandomWorker();
            if(worker == this){ //TODO: fix logic
                pthread_yield();
                return nullptr;
            }

            if(!worker){
                pthread_yield();
                return nullptr;
            }

            return worker->queue_.Steal();
        }
    }

    void* JobPoolWorker::HandleThread(void* data){
        LOG(INFO) << "starting worker....";
        JobPoolWorker* worker = (JobPoolWorker*)data;
        worker->SetState(JobPoolWorker::kRunning);
        while(worker->IsRunning()){
            Job* next = worker->GetNextJob();
            if(next){
                if(!next->Run()){
                    LOG(WARNING) << "couldn't run the \"" << next->GetName() << "\" job.";
                    continue;
                }
                LOG(INFO) << next->GetName() << " finished run w/ the following result: " << next->GetResult();
            }
        }

        return nullptr;
    }
}