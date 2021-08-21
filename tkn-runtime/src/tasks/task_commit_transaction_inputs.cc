#include "task/task_engine.h"
#include "tasks/task_commit_input.h"
#include "tasks/task_commit_transaction.h"
#include "tasks/task_commit_transaction_inputs.h"

namespace token{
  CommitTransactionInputsTask::CommitTransactionInputsTask(CommitTransactionTask* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
    internal::CommitTransactionObjectsTask<Input>(parent, pool, val),
    InputVisitor(){
  }

  bool CommitTransactionInputsTask::Visit(const InputPtr& val){//TODO: better error handling
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    auto task = new CommitInputTask(this, val);
    if(!task->Submit(queue)){
      LOG(ERROR) << "cannot submit new CommitOutputTask.";
      return false;
    }
    return true;
  }
}