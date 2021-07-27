#ifndef TKN_TASK_PROCESS_INPUT_LIST_H
#define TKN_TASK_PROCESS_INPUT_LIST_H

#include "task/task.h"
#include "transaction_input.h"

namespace token{
  namespace task{
    class ProcessTransactionTask;
    class ProcessInputListTask : public task::Task{
    private:
      InputList inputs_;
    public:
      explicit ProcessInputListTask(ProcessTransactionTask* parent, const Hash& transaction, const int64_t& index, InputList& inputs);
      ~ProcessInputListTask() override = default;

      std::string GetName() const override{
        return "ProcessInputListTask";
      }

      void DoWork() override;
    };
  }
}

#endif//TKN_TASK_PROCESS_INPUT_LIST_H