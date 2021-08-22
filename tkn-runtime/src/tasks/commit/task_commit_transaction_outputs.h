#ifndef TKN_TASK_COMMIT_TRANSACTION_OUTPUTS_H
#define TKN_TASK_COMMIT_TRANSACTION_OUTPUTS_H

#include "tasks/commit/task_commit_transaction_objects.h"

namespace token{
  class CommitTransactionTask;
  class CommitTransactionOutputsTask : public internal::CommitTransactionObjectsTask<Output>,
                                       public OutputVisitor{
  private:
    uint64_t current_;
  public:
    CommitTransactionOutputsTask(CommitTransactionTask* parent, internal::WriteBatchList& batches, const IndexedTransactionPtr& val);
    ~CommitTransactionOutputsTask() override = default;
    bool Visit(const OutputPtr& val) override;
    DECLARE_TASK_TYPE(CommitTransactionOutputs);
  };
}

#endif//TKN_TASK_COMMIT_TRANSACTION_OUTPUTS_H