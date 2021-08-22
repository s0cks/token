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
    std::shared_ptr<CommitTransactionInputsTask> commit_inputs_;
    std::shared_ptr<CommitTransactionOutputsTask> commit_outputs_;
  public:
    CommitTransactionTask(task::TaskEngine* engine, internal::WriteBatchList& batches, const IndexedTransactionPtr& val):
      task::Task(engine),
      batches_(batches),
      transaction_(val),
      commit_inputs_(CommitTransactionInputsTask::NewInstance(this, batches_, val)),
      commit_outputs_(CommitTransactionOutputsTask::NewInstance(this, batches_, val)){
    }
    ~CommitTransactionTask() override = default;

    std::string GetName() const override{
      return "CommitTransactionTask()";
    }

    IndexedTransactionPtr GetTransaction() const{
      return transaction_;
    }

    std::shared_ptr<CommitTransactionInputsTask>
    GetCommitInputs() const{
      return commit_inputs_;
    }

    std::shared_ptr<CommitTransactionOutputsTask>
    GetCommitOutputs() const{
      return commit_outputs_;
    }

    void DoWork() override;
  };
}

#endif//TKN_TASKS_COMMIT_TRANSACTION_H