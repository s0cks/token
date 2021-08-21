#include "task/task_engine.h"
#include "tasks/task_verify_output.h"
#include "tasks/task_verify_transaction.h"
#include "tasks/task_verify_transaction_outputs.h"

namespace token{
  VerifyTransactionOutputsTask::VerifyTransactionOutputsTask(VerifyTransactionTask* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
    internal::VerifyTransactionObjectsTask<Output>(parent, pool, val){
  }

  bool VerifyTransactionOutputsTask::Visit(const OutputPtr& val){
    DVLOG(2) << "visiting " << val->ToString() << "....";
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    auto task = new VerifyOutputTask(this, val, pool());
    if(!queue.Push(reinterpret_cast<uword>(task))){
      LOG(ERROR) << "cannot push new VerifyOutputTask.";
      return false;
    }
    return true;
  }
}