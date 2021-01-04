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
    workers_.reserve(FLAGS_num_worker_threads);
    for(int idx = 0; idx < FLAGS_num_worker_threads; idx++){
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
    for(int idx = 0; idx < FLAGS_num_worker_threads; idx++){
      if(pthread_equal(workers_[idx]->GetThreadID(), thread))
        return workers_[idx];
    }
    LOG(INFO) << "cannot find worker: " << thread;
    return nullptr;
  }

  JobWorker* JobScheduler::GetThreadWorker(){
    return GetWorker(pthread_self());
  }

  JobWorker* JobScheduler::GetRandomWorker(){
    std::uniform_int_distribution<int> distribution(0, FLAGS_num_worker_threads - 1);
    int idx = distribution(engine);
    JobWorker* worker = workers_[idx];
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
      for(int idx = 0; idx < FLAGS_num_worker_threads; idx++){
        JobWorker* worker = workers_[idx];
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