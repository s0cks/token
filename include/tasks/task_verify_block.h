#ifndef TKN_TASK_VERIFY_BLOCK_H
#define TKN_TASK_VERIFY_BLOCK_H

#include "block.h"

#include <utility>
#include "task/task.h"

namespace token{
  namespace task{
    class VerifyBlockTask : public Task{
     private:
      BlockPtr block_;
     public:
      VerifyBlockTask(Task* parent, BlockPtr value):
        Task(parent),
        block_(std::move(value)){}
      explicit VerifyBlockTask(const BlockPtr& value):
        VerifyBlockTask(nullptr, value){}
      ~VerifyBlockTask() override = default;

      BlockPtr GetBlock() const{
        return block_;
      }

      std::string GetName() const override{
        return "VerifyBlockTask";
      }

      void DoWork() override;
    };
  }
}

#endif//TKN_TASK_VERIFY_BLOCK_H