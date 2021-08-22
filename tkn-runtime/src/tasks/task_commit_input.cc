#include "tasks/task_commit_input.h"
#include "tasks/task_commit_transaction_inputs.h"

namespace token{
  CommitInputTask::CommitInputTask(CommitTransactionInputsTask* parent, internal::WriteBatchList& batches, InputPtr val):
    task::Task(parent),
    value_(std::move(val)),
    batch_(batches.CreateNewBatch()){
  }

  void CommitInputTask::DoWork(){
    auto hash = value_->hash();
    batch_->Delete(hash);
  }
}