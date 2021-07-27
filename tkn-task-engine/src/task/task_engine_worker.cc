#include "task/task_engine.h"
#include "task/task_engine_worker.h"

namespace token{
  namespace task{
    Task* TaskEngineWorker::GetNextTask(){
      auto next = (Task*)GetTaskQueue().Pop();
      if(next)
        return next;
      auto queue = GetEngine()->GetRandomQueue();
      if(!queue || !queue->queue || platform::ThreadEquals(GetThreadId(), queue->thread)){
        pthread_yield();//TODO: convert to platform agnostic
        return nullptr;
      }
      return (Task*)queue->queue->Steal();
    }

    void TaskEngineWorker::HandleThread(uword parameter){
      auto worker = (TaskEngineWorker *) parameter;
      auto &worker_id = worker->GetWorkerId();

      worker->SetState(State::kStarting);
      LOG(INFO) << "starting task engine worker #" << (worker_id) << "....";
      worker->GetEngine()->RegisterQueue(worker->GetTaskQueue());
      worker->SetState(State::kIdle);
      do{
        auto next = worker->GetNextTask();
        if(!next)
          continue;

        auto name = next->GetName();
        LOG(INFO) << "task engine worker #" << (worker_id) << " is running " << name << "....";

        auto start_ms = platform::Clock::now();
        if (!next->Run()) {
          LOG(WARNING) << "couldn't run the " << (name) << " job.";
          continue;
        }
        auto end_ms = platform::Clock::now();

        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_ms - start_ms);
        LOG(INFO) << "task engine worker #" << (worker_id) << " has finished " << (name) << " (" << (duration_ms.count()) << "ms)";
      } while (!worker->IsStopping());
      LOG(INFO) << "task engine worker #" << (worker_id) << " is stopping.";
      worker->SetState(State::kStopped);
      LOG(INFO) << "task engine worker #" << (worker_id) << " is stopped.";
      pthread_exit(nullptr);//TODO: proper exit
    }
  }
}