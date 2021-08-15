#ifndef TKN_TASK_PROCESS_OUTPUT_LIST_H
#define TKN_TASK_PROCESS_OUTPUT_LIST_H

#include "batch.h"
#include "transaction_output.h"

namespace token{
  namespace task{
    class ProcessTransactionTask;
    class ProcessOutputListTask : public task::Task{
    private:
      internal::PoolWriteBatch batch_;
      ObjectPool& pool_;
      atomic::RelaxedAtomic<uint64_t>& counter_;
      Hash transaction_;
      int64_t index_;
      std::vector<Output> outputs_;
    public:
      explicit ProcessOutputListTask(ProcessTransactionTask* parent, ObjectPool& pool, const Hash& transaction, const int64_t& index, std::vector<Output>& outputs);
      ~ProcessOutputListTask() override = default;

      std::string GetName() const override{
        return "ProcessOutputListTask";
      }

      void DoWork() override;
    };
  }
}

#endif//TKN_TASK_PROCESS_OUTPUT_LIST_H