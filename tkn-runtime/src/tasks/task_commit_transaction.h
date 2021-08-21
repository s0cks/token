#ifndef TKN_TASKS_COMMIT_TRANSACTION_H
#define TKN_TASKS_COMMIT_TRANSACTION_H

#include "tasks/task_commit_transaction_inputs.h"
#include "tasks/task_commit_transaction_outputs.h"

namespace token{
  class CommitTransactionTask : public task::Task{
  private:
    UnclaimedTransactionPool& pool_;
    IndexedTransactionPtr transaction_;
    std::shared_ptr<CommitTransactionInputsTask> commit_inputs_;
    std::shared_ptr<CommitTransactionOutputsTask> commit_outputs_;

    inline UnclaimedTransactionPool&
    pool() const{
      return pool_;
    }
  public:
    CommitTransactionTask(task::TaskEngine* engine, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
      task::Task(engine),
      pool_(pool),
      transaction_(val),
      commit_inputs_(std::make_shared<CommitTransactionInputsTask>(this, pool, val)),
      commit_outputs_(std::make_shared<CommitTransactionOutputsTask>(this, pool, val)){
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