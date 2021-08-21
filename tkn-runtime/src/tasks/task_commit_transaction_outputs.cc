#include "task/task_engine.h"
#include "tasks/task_commit_output.h"
#include "tasks/task_commit_transaction.h"
#include "tasks/task_commit_transaction_outputs.h"

namespace token{
  CommitTransactionOutputsTask::CommitTransactionOutputsTask(CommitTransactionTask* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
    internal::CommitTransactionObjectsTask<Output>(parent, pool, val),
    OutputVisitor(){
  }

  bool CommitTransactionOutputsTask::Visit(const OutputPtr& val){//TODO: better error handling
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    auto task = new CommitOutputTask(this, val);
    if(!task->Submit(queue)){
      LOG(ERROR) << "cannot submit new CommitOutputTask().";
      return false;
    }
    return true;
  }
}