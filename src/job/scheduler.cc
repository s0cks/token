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

  JobWorker* JobScheduler::GetRandomWorker(){
    std::uniform_int_distribution<int> distribution(0, FLAGS_num_workers - 1);
    int idx = distribution(engine);
    JobWorker* worker = workers_[idx];
    return worker && worker->IsRunning() ? worker : nullptr;
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

  bool JobScheduler::GetStats(Json::Writer& writer){
    int64_t total_ran = 0;
    int64_t total_discarded = 0;

    writer.StartObject();
    {
      for(int idx = 0; idx < FLAGS_num_workers; idx++){
        JobWorker* worker = workers_[idx];

        char name[10];
        snprintf(name, 10, "worker-%02" PRId32, idx);

        if(!worker || !worker->IsRunning()){
          Json::SetFieldNull(writer, name);
          continue;
        }

        int64_t num_ran = worker->GetJobsRan()->Get();
        int64_t num_discarded = worker->GetJobsDiscarded()->Get();

        std::shared_ptr<Metrics::Snapshot> snapshot = worker->GetHistogram()->GetSnapshot();
        writer.Key(name, 9);
        writer.StartObject();
        {
          Json::SetField(writer, "total_ran", num_ran);
          Json::SetField(writer, "total_discarded", num_discarded);
          //TODO: refactor fields?
          Json::SetField(writer, "MinTime", snapshot->GetMin());
          Json::SetField(writer, "MeanTime", snapshot->GetMean());
          Json::SetField(writer, "MaxTime", snapshot->GetMax());
        }
        writer.EndObject();

        total_ran += num_ran;
        total_discarded += num_discarded;
      }

      writer.Key("overall");
      writer.StartObject();
      {
        Json::SetField(writer, "total_ran", total_ran);
        Json::SetField(writer, "total_discarded", total_discarded);
      }
      writer.EndObject();
    }
    writer.EndObject();
    return true;
  }
}