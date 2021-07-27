#ifndef TKN_TASK_PROCESS_OUTPUT_LIST_H
#define TKN_TASK_PROCESS_OUTPUT_LIST_H

#include "task/task.h"
#include "transaction_output.h"

namespace token{
  namespace task{
    class ProcessTransactionTask;
    class ProcessOutputListTask : public task::Task{
    private:
      leveldb::WriteBatch batch_;
      Hash transaction_;
      int64_t index_;
      OutputList outputs_;
    public:
      explicit ProcessOutputListTask(ProcessTransactionTask* parent, const Hash& transaction, const int64_t& index, OutputList& outputs);
      ~ProcessOutputListTask() override = default;

      std::string GetName() const override{
        return "ProcessOutputListTask";
      }

      void DoWork() override;
    };
  }
}

#endif//TKN_TASK_PROCESS_OUTPUT_LIST_H