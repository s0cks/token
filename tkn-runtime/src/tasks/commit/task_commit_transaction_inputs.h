#ifndef TKN_TASK_COMMIT_TRANSACTION_INPUTS_H
#define TKN_TASK_COMMIT_TRANSACTION_INPUTS_H

#include "tasks/commit/task_commit_transaction_objects.h"

namespace token{
  class CommitTransactionInputsTask;
  typedef std::shared_ptr<CommitTransactionInputsTask> CommitTransactionInputsTaskPtr;

  class CommitTransactionTask;
  class CommitTransactionInputsTask : public internal::CommitTransactionObjectsTask<Input>,
                                      public InputVisitor{
  public:
    CommitTransactionInputsTask(CommitTransactionTask* parent, internal::WriteBatchList& batches, const IndexedTransactionPtr& val);
    ~CommitTransactionInputsTask() override = default;

    bool Visit(const InputPtr& val) override;
    DECLARE_TASK_TYPE(CommitTransactionInputs);
  };
}

#endif//TKN_TASK_COMMIT_TRANSACTION_INPUTS_H