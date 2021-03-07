#include <random>
#include <vector>
#include <glog/logging.h>

#include "job/job.h"
#include "job/scheduler.h"
#include "job/worker.h"

namespace token{
  const int32_t JobScheduler::kMaxNumberOfJobs = 1024;

  static std::default_random_engine engine;
  static std::map<ThreadId, JobQueue*> queues_;
  static std::vector<JobWorker*> workers_;

  bool JobScheduler::RegisterQueue(const ThreadId& thread, JobQueue* queue){
    auto pos = queues_.insert({ thread, queue });
    return pos.second;
  }

  bool JobScheduler::Initialize(){
    workers_.reserve(FLAGS_num_workers);
    for(int idx = 0; idx < FLAGS_num_workers; idx++){
      JobWorker* worker = new JobWorker(idx, JobScheduler::kMaxNumberOfJobs);
      workers_.push_back(worker);
      if(!worker->Start()){
        LOG(WARNING) << "couldn't start job pool worker #" << idx;
        return false;
      }

      if(!queues_.insert({ worker->GetThreadID(), worker->GetQueue() }).second){
        LOG(WARNING) << "couldn't register worker #" << idx << " queue.";
        return false;
      }
    }
    return true;
  }

  bool JobScheduler::Schedule(Job* job){
    return GetRandomQueue()->Push(job);
  }

  JobQueue* JobScheduler::GetWorker(const ThreadId& thread){
    for(auto& it : queues_)
      if(pthread_equal(it.first, thread))
        return it.second;
    LOG(INFO) << "cannot find worker: " << thread;
    return nullptr;
  }

  JobQueue* JobScheduler::GetThreadQueue(){
    return GetWorker(pthread_self());
  }

  JobQueue* JobScheduler::GetRandomQueue(){
    std::vector<ThreadId> keys;
    for(auto& it : queues_)
      keys.push_back(it.first);
    std::uniform_int_distribution<int> distribution(0, keys.size() - 1);
    int idx = distribution(engine);

    ThreadId key = keys[idx];
    auto pos = queues_.find(key);
    if(pos == queues_.end())
      return nullptr;
    return pos->second;
  }
}