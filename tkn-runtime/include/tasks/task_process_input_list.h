#ifndef TKN_TASK_PROCESS_INPUT_LIST_H
#define TKN_TASK_PROCESS_INPUT_LIST_H

#include "batch.h"
#include "transaction_input.h"
#include "tasks/task_batch_write.h"

namespace token{
  namespace task{
    class ProcessTransactionTask;
    class ProcessInputListTask : public task::Task{
    private:
      ObjectPool& pool_;
      internal::WriteBatch batch_;
      std::vector<std::shared_ptr<Input>> inputs_;
    public:
      explicit ProcessInputListTask(ProcessTransactionTask* parent, ObjectPool& pool, const Hash& transaction, const int64_t& index, std::vector<std::shared_ptr<Input>>& inputs);
      ~ProcessInputListTask() override = default;

      std::string GetName() const override{
        return "ProcessInputListTask";
      }

      void DoWork() override;
    };
  }
}

#endif//TKN_TASK_PROCESS_INPUT_LIST_H