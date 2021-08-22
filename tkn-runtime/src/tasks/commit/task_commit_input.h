#ifndef TKN_TASK_COMMIT_INPUT_H
#define TKN_TASK_COMMIT_INPUT_H

#include "batch.h"
#include "task/task.h"
#include "transaction_input.h"

namespace token{
  class CommitTransactionInputsTask;
  class CommitInputTask : public task::Task{
  private:
    InputPtr value_;
    std::shared_ptr<internal::WriteBatch> batch_;
  public:
    CommitInputTask(CommitTransactionInputsTask* parent, internal::WriteBatchList& batches, InputPtr val);
    ~CommitInputTask() override = default;

    std::string GetName() const override{
      return "CommitInputTask()";
    }

    void DoWork() override;
  };
}

#endif//TKN_TASK_COMMIT_INPUT_H