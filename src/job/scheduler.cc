#include <random>
#include <vector>
#include <glog/logging.h>

#include "job/job.h"
#include "job/scheduler.h"
#include "job/worker.h"

namespace Token{
  const int32_t JobScheduler::kMaxNumberOfJobs = 1024;
  const int16_t JobScheduler::kMaxNumberOfWorkers = 10;

  static std::default_random_engine engine;
  static JobWorker* workers_ = nullptr;

  bool JobScheduler::Initialize(){
    workers_ = (JobWorker*) malloc(sizeof(JobWorker) * kMaxNumberOfWorkers);
    for(int idx = 0; idx < JobScheduler::kMaxNumberOfWorkers; idx++){
      JobWorker* worker = new(&workers_[idx]) JobWorker(idx, JobScheduler::kMaxNumberOfJobs);
      if(!worker->Start()){
        LOG(WARNING) << "couldn't start job pool worker #" << idx;
        return false;
      }
    }
    return true;
  }

  bool JobScheduler::Schedule(Job* job){
    return GetRandomWorker()->Schedule(job);
  }

  JobWorker* JobScheduler::GetWorker(const std::thread::id& thread){
    for(int idx = 0; idx < JobScheduler::kMaxNumberOfWorkers; idx++){
      if(workers_[idx].GetThreadID() == thread)
        return &workers_[idx];
    }
    LOG(INFO) << "cannot find worker: " << thread;
    return nullptr;
  }

  JobWorker* JobScheduler::GetThreadWorker(){
    return GetWorker(std::this_thread::get_id());
  }

  JobWorker* JobScheduler::GetRandomWorker(){
    std::uniform_int_distribution<int> distribution(0, JobScheduler::kMaxNumberOfWorkers);

    int idx = distribution(engine);
    JobWorker* worker = &workers_[idx];
    return (worker && worker->IsRunning()) ? worker : nullptr;
  }

  void JobScheduler::PrintWorkerStatistics(){
    for(int idx = 0; idx < JobScheduler::kMaxNumberOfWorkers; idx++){
      JobWorker* worker = &workers_[idx];
      std::shared_ptr<Metrics::Snapshot> snapshot = worker->GetHistogram()->GetSnapshot();
      LOG(INFO) << "worker #" << worker->GetWorkerID() << " statistics:";
      LOG(INFO) << " - min time: " << snapshot->GetMin();
      LOG(INFO) << " - max time: " << snapshot->GetMax();
      LOG(INFO) << " - mean time: " << snapshot->GetMean();
    }
  }
}