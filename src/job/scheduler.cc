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

  static inline std::string
  GetWorkerID(JobWorker* worker){
    std::stringstream ss;
    ss << "worker-" << worker->GetWorkerID();
    return ss.str();
  }

  bool JobScheduler::GetWorkerStatistics(JsonString& json){
    JsonWriter writer(json);
    writer.StartObject();
    {
      for(int idx = 0; idx < JobScheduler::kMaxNumberOfWorkers; idx++){
        JobWorker* worker = &workers_[idx];
        std::string worker_id = GetWorkerID(worker);
        if(!worker || !worker->IsRunning()){
          SetFieldNull(writer, worker_id);
          continue;
        }

        std::shared_ptr<Metrics::Snapshot> snapshot = worker->GetHistogram()->GetSnapshot();
        writer.Key(worker_id.data(), worker_id.length());
        writer.StartObject();
        {
          SetField(writer, "NumberOfJobsRan", worker->GetJobsRan()->Get());
          SetField(writer, "NumberOfJobsDiscarded", worker->GetJobsDiscarded()->Get());

          writer.Key("RuntimeMilliseconds");
          writer.StartObject();
          {
            SetField(writer, "Min", snapshot->GetMin());
            SetField(writer, "Mean", snapshot->GetMean());
            SetField(writer, "Max", snapshot->GetMax());
          }
          writer.EndObject();
        }
        writer.EndObject();
      }
    }
    writer.EndObject();
    return true;
  }
}