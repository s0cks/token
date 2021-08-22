#include "task/task_engine.h"
#include "tasks/commit/task_commit_input.h"
#include "tasks/commit/task_commit_transaction.h"
#include "tasks/commit/task_commit_transaction_inputs.h"

namespace token{
  CommitTransactionInputsTask::CommitTransactionInputsTask(CommitTransactionTask* parent, internal::WriteBatchList& batches, const IndexedTransactionPtr& val):
    internal::CommitTransactionObjectsTask<Input>(parent, batches, val),
    InputVisitor(){
  }

  bool CommitTransactionInputsTask::Visit(const InputPtr& val){//TODO: better error handling
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    auto task = new CommitInputTask(this, batches_, val);
    if(!task->Submit(queue)){
      LOG(ERROR) << "cannot submit new CommitOutputTask.";
      return false;
    }
    return true;
  }

  void CommitTransactionInputsTask::DoWork(){
    DVLOG(2) << "committing transaction " << hash() << " inputs....";
    if(!transaction_->VisitInputs(this)){
      LOG(ERROR) << "cannot visit transaction inputs.";
      return;
    }
  }

  std::string CommitTransactionInputsTask::ToString() const{
    std::stringstream ss;
    ss << "CommitTransactionInputsTask(";
    ss << "value=" << transaction_->ToString();
    ss << ")";
    return ss.str();
  }
}