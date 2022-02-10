#ifndef TKN_TASK_COMMIT_INPUT_H
#define TKN_TASK_COMMIT_INPUT_H

#include "batch.h"
#include "task/task.h"
#include "../../../../Sources/token/input.h"

namespace token{
  class CommitTransactionInputsTask;
  class CommitInputTask : public task::Task{
  private:
    InputPtr value_;
    std::shared_ptr<internal::WriteBatch> batch_;
  public:
    CommitInputTask(CommitTransactionInputsTask* parent, internal::WriteBatchList& batches, InputPtr val);
    ~CommitInputTask() override = default;
    DECLARE_TASK_TYPE(CommitInput);
  };
}

#endif//TKN_TASK_COMMIT_INPUT_H