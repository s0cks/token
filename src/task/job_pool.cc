#include <vector>
#include "task/job_pool_worker.h"

namespace Token{
    static std::vector<JobPoolWorker*> workers_;
    static JobQueue* queue_ = nullptr;

    bool JobPool::Initialize(){
        queue_ = new JobQueue(1024);
        for(int idx = 0; idx < JobPool::kMaxNumberOfWorkers; idx++){
            JobPoolWorker* worker = new JobPoolWorker(queue_);
            worker->Start();
        }
        return true;
    }

    bool JobPool::Schedule(Job* job){
        return queue_->Push(job);
    }
}