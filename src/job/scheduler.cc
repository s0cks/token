#include <random>
#include <vector>
#include <glog/logging.h>

#include "job/job.h"
#include "job/scheduler.h"
#include "job/worker.h"

namespace Token{
  const int32_t JobScheduler::kMaxNumberOfJobs = 1024;

  static std::default_random_engine engine;
  static std::vector<JobWorker*> workers_;

  bool JobScheduler::Initialize(){
    workers_.reserve(FLAGS_num_workers);
    for(int idx = 0; idx < FLAGS_num_workers; idx++){
      workers_[idx] = new JobWorker(idx, JobScheduler::kMaxNumberOfJobs);
      if(!workers_[idx]->Start()){
        LOG(WARNING) << "couldn't start job pool worker #" << idx;
        return false;
      }
    }
    return true;
  }

  bool JobScheduler::Schedule(Job* job){
    return GetRandomWorker()->Schedule(job);
  }

  JobWorker* JobScheduler::GetWorker(const ThreadId& thread){
    for(int idx = 0; idx < FLAGS_num_workers; idx++){
      if(pthread_equal(workers_[idx]->GetThreadID(), thread)){
        return workers_[idx];
      }
    }
    LOG(INFO) << "cannot find worker: " << thread;
    return nullptr;
  }

  JobWorker* JobScheduler::GetThreadWorker(){
    return GetWorker(pthread_self());
  }

  JobWorker* JobScheduler::GetRandomWorker(){
    std::uniform_int_distribution<int> distribution(0, FLAGS_num_workers - 1);
    int idx = distribution(engine);
    JobWorker* worker = workers_[idx];
    return (worker && worker->IsRunning()) ? worker : nullptr;
  }

  JobSchedulerStats JobScheduler::GetStats(){
    JobWorkerStats workers[FLAGS_num_workers];
    for(int idx = 0; idx < FLAGS_num_workers; idx++){
      JobWorker* worker = workers_[idx];
      if(!worker || !worker->IsRunning())
        continue;
      workers[idx] = worker->GetStats();
    }
    return JobSchedulerStats(workers);
  }

  bool JobScheduler::GetWorkerStatistics(JsonString& json){
    JsonWriter writer(json);
    writer.StartObject();
    {
      for(int idx = 0; idx < FLAGS_num_workers; idx++){
        JobWorker* worker = workers_[idx];

        char name[10];
        snprintf(name, 10, "worker-%02d", idx);

        if(!worker || !worker->IsRunning()){
          SetFieldNull(writer, name);
          continue;
        }

        std::shared_ptr<Metrics::Snapshot> snapshot = worker->GetHistogram()->GetSnapshot();
        writer.Key(name, 9);
        writer.StartObject();
        {
          SetField(writer, "NumberOfJobsRan", worker->GetJobsRan()->Get());
          SetField(writer, "NumberOfJobsDiscarded", worker->GetJobsDiscarded()->Get());
          SetField(writer, "MinTime", snapshot->GetMin());
          SetField(writer, "MeanTime", snapshot->GetMean());
          SetField(writer, "MaxTime", snapshot->GetMax());
        }
        writer.EndObject();
      }
    }
    writer.EndObject();
    return true;
  }
}