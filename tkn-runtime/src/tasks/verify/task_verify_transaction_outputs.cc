#include "task/task_engine.h"
#include "tasks/verify/task_verify_output.h"
#include "tasks/verify/task_verify_transaction.h"
#include "tasks/verify/task_verify_transaction_outputs.h"

namespace token{
  VerifyTransactionOutputsTask::VerifyTransactionOutputsTask(VerifyTransactionTask* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
    internal::VerifyTransactionObjectsTask<Output>(parent, pool, val){
  }

  bool VerifyTransactionOutputsTask::Visit(const OutputPtr& val){
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    auto task = new VerifyOutputTask(this, val, pool());
    if(!queue.Push(reinterpret_cast<uword>(task))){
      LOG(ERROR) << "cannot push new VerifyOutputTask.";
      return false;
    }
    return true;
  }

  void VerifyTransactionOutputsTask::DoWork(){
    DVLOG(1) << "verifying transaction " << hash() << " outputs....";
    if(!transaction_->VisitOutputs(this)){
      LOG(ERROR) << "cannot visit transaction inputs.";
      return;
    }
  }

  std::string VerifyTransactionOutputsTask::ToString() const{
    std::stringstream ss;
    ss << "VerifyTransactionOutputsTask(";
    ss << "value=" << transaction_->hash();
    ss << ")";
    return ss.str();
  }
}