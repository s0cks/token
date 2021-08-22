#include "task/task_engine.h"
#include "tasks/verify/task_verify_input.h"
#include "tasks/verify/task_verify_transaction.h"
#include "tasks/verify/task_verify_transaction_inputs.h"

namespace token{
  VerifyTransactionInputsTask::VerifyTransactionInputsTask(VerifyTransactionTask* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
    internal::VerifyTransactionObjectsTask<Input>(parent, pool, val){}

  bool VerifyTransactionInputsTask::Visit(const InputPtr& val){
    DVLOG(2) << "visiting " << val->ToString() << "....";
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    auto task = new VerifyInputTask(this, val, pool());
    if(!queue.Push(reinterpret_cast<uword>(task))){
      LOG(ERROR) << "cannot push new VerifyInputTask";
      return false;
    }
    return true;
  }

  void VerifyTransactionInputsTask::DoWork(){
    DVLOG(1) << "verifying transaction " << hash() << " inputs....";
    if(!transaction_->VisitInputs(this)){
      LOG(ERROR) << "cannot visit transaction inputs.";
      return;
    }
  }

  std::string VerifyTransactionInputsTask::ToString() const{
    std::stringstream ss;
    ss << "VerifyTransactionInputsTask(";
    ss << "value=" << transaction_->hash();
    ss << ")";
    return ss.str();
  }
}