#ifndef TKN_TASK_ENGINE_H
#define TKN_TASK_ENGINE_H

#include <random>
#include <vector>
#include <glog/logging.h>

#include "task/task.h"
#include "task/task_queue.h"
#include "task/task_engine_worker.h"

namespace token{
  namespace task{
    struct TaskEngineQueue{
      TaskEngine* engine;
      ThreadId thread;
      TaskQueue* queue;

      TaskEngineQueue():
        engine(nullptr),
        thread(),
        queue(nullptr){}
      TaskEngineQueue(TaskEngine* e, const ThreadId& thr, TaskQueue* q):
        engine(e),
        thread(thr),
        queue(q){}
      TaskEngineQueue(const TaskEngineQueue& rhs) = default;
      ~TaskEngineQueue() = default;
      TaskEngineQueue& operator=(const TaskEngineQueue& rhs) = default;
    };

    static const int64_t kDefaultTaskEngineQueueSize = 1024;
    static const int64_t kDefaultTaskEngineWorkers = 2;
    static const int64_t kMaximumTaskEngineWorkers = 128; //TODO: make platform specific
    static const int64_t kMinimumTaskEngineWorkers = 0;

    class TaskEngine{
     protected:
      std::random_device random_device_;
      std::default_random_engine random_engine_;

      TaskEngineWorker** workers_;
      int64_t workers_size_;

      std::vector<TaskEngineQueue> queues_;

      inline size_t
      GetRandomQueueIndex(){
        std::uniform_int_distribution<size_t> distribution(0, queues_.size() - 1);
        size_t qidx = distribution(random_engine_);
        return qidx;
      }

      inline void
      RegisterQueue(const ThreadId& thread, TaskQueue* queue){
        queues_.emplace_back(this, thread, queue);
      }
     public:
      TaskEngine(const int64_t& num_workers, const int64_t& num_queues, const int64_t& max_queue_size);
      TaskEngine(const int64_t& num_workers, const int64_t& num_queues):
        TaskEngine(num_workers, num_queues, kDefaultTaskEngineQueueSize){}
      TaskEngine(const TaskEngine& other) = delete;
      ~TaskEngine(){
        delete[] workers_;
      }

      size_t GetNumberOfQueues(){
        return queues_.size();
      }

      void Shutdown();
      TaskEngineQueue* GetQueue(const size_t& qidx);
      TaskEngineWorker* GetWorker(const ThreadId& thread);

      TaskEngineWorker* GetCurrentWorker(){
        return GetWorker(platform::GetCurrentThreadId());
      }

      inline TaskEngineQueue*
      GetRandomQueue(){
        return GetQueue(GetRandomQueueIndex());
      }

      void RegisterQueue(TaskQueue& queue){
        return RegisterQueue(platform::GetCurrentThreadId(), &queue);
      }

      TaskEngine& operator=(const TaskEngine& rhs) = delete;
    };
  }
}

#endif//TKN_TASK_ENGINE_H