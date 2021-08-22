#include "common.h"
#include "task/task_engine.h"
#include "task/task_engine_worker.h"

namespace token{
  namespace task{
    TaskEngine::TaskEngine(const int64_t &num_workers, const int64_t& num_queues, const uint64_t& max_queue_size):
      random_device_(),
      random_engine_(random_device_()),
      workers_(new TaskEngineWorker*[num_workers]),
      workers_size_(num_workers),
      queues_(num_workers+num_queues){
      for(auto idx = 0; idx < num_workers; idx++){
        auto worker = new TaskEngineWorker(this, idx, RoundUpPowTwo(max_queue_size));
        workers_[idx] = worker;
        if(!worker->Start()){
          LOG(FATAL) << "cannot start task engine worker #" << (idx + 1);
          return;
        }
      }
    }

    void TaskEngine::Shutdown(){
      for(auto idx = 0; idx < workers_size_; idx++){
        auto worker = workers_[idx];
        if(!worker->Shutdown())
          LOG(ERROR) << "cannot shutdown task engine worker #" << (idx+1);
        delete worker;
      }
    }

    TaskEngineQueue* TaskEngine::GetQueue(const size_t& qidx){
      if(qidx < 0 || qidx > queues_.size()){
        LOG(WARNING) << "cannot get task queue #" << qidx;
        return nullptr;
      }
      return &queues_[qidx];
    }

    TaskEngineWorker* TaskEngine::GetWorker(const ThreadId& thread){
      for(auto idx = 0; idx < workers_size_; idx++){
        auto worker = workers_[idx];
        if(platform::ThreadEquals(thread, worker->GetThreadId()))
          return worker;
      }
      return nullptr;
    }
  }
}