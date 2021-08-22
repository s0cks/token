#include "task/task_engine.h"
#include "tasks/commit/task_commit_output.h"
#include "tasks/commit/task_commit_transaction.h"
#include "tasks/commit/task_commit_transaction_outputs.h"

namespace token{
  CommitTransactionOutputsTask::CommitTransactionOutputsTask(CommitTransactionTask* parent, internal::WriteBatchList& batches, const IndexedTransactionPtr& val):
    internal::CommitTransactionObjectsTask<Output>(parent, batches, val),
    OutputVisitor(),
    current_(0){
  }

  bool CommitTransactionOutputsTask::Visit(const OutputPtr& val){//TODO: better error handling
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    auto source = TransactionReference(hash(), current_++);
    auto task = new CommitOutputTask(this, batches_, source, val);
    if(!task->Submit(queue)){
      LOG(ERROR) << "cannot submit new CommitOutputTask().";
      return false;
    }
    return true;
  }

  void CommitTransactionOutputsTask::DoWork(){
    DVLOG(2) << "committing transaction " << hash() << " outputs....";
    if(!transaction_->VisitOutputs(this)){
      LOG(ERROR) << "cannot visit transaction outputs.";
      return;
    }
  }

  std::string CommitTransactionOutputsTask::ToString() const{
    std::stringstream ss;
    ss << "CommitTransactionOutputsTask(";
    ss << "value=" << transaction_->hash();
    ss << ")";
    return ss.str();
  }
}