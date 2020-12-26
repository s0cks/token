#include <random>
#include <vector>
#include <glog/logging.h>

#include "job/job.h"
#include "job/scheduler.h"
#include "job/worker.h"

namespace Token{
    static std::default_random_engine engine;
    static std::vector<JobPoolWorker*> workers_;

    bool JobScheduler::Initialize(){
        for(int idx = 0; idx < JobScheduler::kMaxNumberOfWorkers; idx++){
            JobPoolWorker* worker = new JobPoolWorker(JobScheduler::kMaxNumberOfJobs);
            if(!worker->Start()){
                LOG(WARNING) << "couldn't start job pool worker #" << idx;
                return false;
            }
            workers_.push_back(worker);
        }
        return true;
    }

    bool JobScheduler::Schedule(Job* job){
        return GetRandomWorker()->Schedule(job);
    }

    JobPoolWorker* JobScheduler::GetWorker(pthread_t wthread){
        for(auto& worker : workers_){
            if(worker->GetThreadID() == wthread)
                return worker;
        }
        return nullptr;
    }

    JobPoolWorker* JobScheduler::GetThreadWorker(){
        return GetWorker(pthread_self());
    }

    JobPoolWorker* JobScheduler::GetRandomWorker(){
        std::uniform_int_distribution<int> distribution(0, workers_.size() - 1);
        JobPoolWorker* worker = workers_[distribution(engine)];
        return worker->IsRunning()
               ? worker
               : nullptr;
    }
}