#ifndef TOKEN_SNAPSHOT_TASK_H
#define TOKEN_SNAPSHOT_TASK_H

#include "task.h"
#include "snapshot.h"

namespace Token{
  class SnapshotTask : public AsyncTask{
   protected:
    SnapshotTask(uv_loop_t *loop):
      AsyncTask(loop){}

    AsyncTaskResult DoWork(){
      LOG(INFO) << "generating new snapshot....";
      if(!Snapshot::WriteNewSnapshot()){
        LOG(WARNING) << "couldn't write new snapshot!";
        return AsyncTaskResult::Failed("Couldn't write new snapshot!");
      }
      return AsyncTaskResult::Success("Snapshot Written!");
    }
   public:
    ~SnapshotTask() = default;

    std::string ToString() const{
      std::stringstream ss;
      ss << "SnapshotTask()";
      return ss.str();
    }

   DEFINE_ASYNC_TASK(Snapshot);

    static SnapshotTask *NewInstance(uv_loop_t *loop = uv_default_loop()){
      return new SnapshotTask(loop);
    }

    static bool ScheduleNewTask(uv_loop_t *loop = uv_default_loop()){
      return NewInstance(loop)->Submit();
    }
  };
}

#endif //TOKEN_SNAPSHOT_TASK_H