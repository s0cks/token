#include "tasks/commit/task_commit_input.h"
#include "tasks/commit/task_commit_transaction_inputs.h"

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

  std::string CommitInputTask::ToString() const{
    std::stringstream ss;
    ss << "CommitInputTask(";
    ss << "value=" << value_->ToString();
    ss << ")";
    return ss.str();
  }
}