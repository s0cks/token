#ifndef TKN_TASK_DEBUG_H
#define TKN_TASK_DEBUG_H

#include <glog/logging.h>
#include "task/task.h"

namespace token{
  namespace task{
    class DebugTask : public Task{
     private:
      int64_t task_number_;
     public:
      explicit DebugTask(const int64_t& task_number):
        Task(),
        task_number_(task_number){}
      ~DebugTask() override = default;

      int64_t GetTaskNumber() const{
        return task_number_;
      }

      std::string GetName() const override{
        return "DebugTask";
      }

      void DoWork() override{
        LOG(INFO) << "task #" << GetTaskNumber() << ": Hello World!";
        SetState(Status::kSuccessful);
      }
    };
  }
}

#endif//TKN_TASK_DEBUG_H