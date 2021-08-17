#include "task/task_engine.h"
#include "tasks/task_verify_input.h"
#include "tasks/task_verify_transaction.h"
#include "tasks/task_verify_transaction_inputs.h"

namespace token{
  VerifyTransactionInputsTask::VerifyTransactionInputsTask(VerifyTransactionTask* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
    internal::VerifyTransactionObjectsTask<Input>(parent, pool, val){}

  bool VerifyTransactionInputsTask::Visit(const Input& val){
    DVLOG(2) << "visiting " << val.ToString() << "....";
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    auto task = new VerifyInputTask(this, val, pool());
    if(!queue.Push(reinterpret_cast<uword>(task))){
      LOG(ERROR) << "cannot push new VerifyInputTask";
      return false;
    }
    return true;
  }
}