#ifndef TKN_TASKS_COMMIT_TRANSACTION_H
#define TKN_TASKS_COMMIT_TRANSACTION_H

#include "batch.h"
#include "atomic/linked_list.h"
#include "tasks/commit/task_commit_transaction_inputs.h"
#include "tasks/commit/task_commit_transaction_outputs.h"

namespace token{
  class CommitTransactionTask : public task::Task{
  private:
    internal::WriteBatchList& batches_;
    IndexedTransactionPtr transaction_;
    CommitTransactionInputsTask* inputs_;
    CommitTransactionOutputsTask* outputs_;
  public:
    CommitTransactionTask(task::TaskEngine* engine, internal::WriteBatchList& batches, const IndexedTransactionPtr& val):
      task::Task(engine),
      batches_(batches),
      transaction_(val),
      inputs_(new CommitTransactionInputsTask(this, batches_, val)),
      outputs_(new CommitTransactionOutputsTask(this, batches_, val)){
    }
    ~CommitTransactionTask() override = default;

    IndexedTransactionPtr GetTransaction() const{
      return transaction_;
    }

#ifdef TOKEN_DEBUG
    void PrintStats();
#endif//TOKEN_DEBUG

    DECLARE_TASK_TYPE(CommitTransaction);
  };
}

#endif//TKN_TASKS_COMMIT_TRANSACTION_H