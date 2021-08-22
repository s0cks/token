#ifndef TKN_TASK_COMMIT_OUTPUT_H
#define TKN_TASK_COMMIT_OUTPUT_H

#include "batch.h"
#include "task/task.h"
#include "transaction_output.h"
#include "transaction_reference.h"

namespace token{
  class CommitTransactionOutputsTask;
  class CommitOutputTask : public task::Task{
  private:
    TransactionReference source_;
    OutputPtr value_;
    std::shared_ptr<internal::WriteBatch> batch_;
  public:
    CommitOutputTask(CommitTransactionOutputsTask* parent, internal::WriteBatchList& batches, const TransactionReference& source, OutputPtr val);
    ~CommitOutputTask() override = default;

    std::string GetName() const override{
      return "CommitOutputTask()";
    }

    void DoWork() override;
  };
}

#endif//TKN_TASK_COMMIT_OUTPUT_H