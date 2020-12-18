#include <vector>
#include "task/job_pool_worker.h"

namespace Token{
    static std::default_random_engine engine;
    static std::vector<JobPoolWorker*> workers_;

    bool JobPool::Initialize(){
        for(int idx = 0; idx < JobPool::kMaxNumberOfWorkers; idx++){
            JobPoolWorker* worker = new JobPoolWorker(JobPool::kMaxNumberOfJobs);
            if(!worker->Start()){
                LOG(WARNING) << "couldn't start job pool worker #" << idx;
                return false;
            }
            workers_.push_back(worker);
        }
        return true;
    }

    bool JobPool::Schedule(Job* job){
        return GetRandomWorker()->Schedule(job);
    }

    JobPoolWorker* JobPool::GetWorker(pthread_t wthread){
        return nullptr; //TODO: implement
    }

    JobPoolWorker* JobPool::GetRandomWorker(){
        std::uniform_int_distribution<int> distribution(0, workers_.size() - 1);
        JobPoolWorker* worker = workers_[distribution(engine)];
        return worker->IsRunning()
             ? worker
             : nullptr;
    }
}