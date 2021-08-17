#include "task/task_engine.h"
#include "tasks/task_verify_transaction.h"

namespace token{
  void VerifyTransactionTask::DoWork(){
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    if(!verify_inputs_->Submit(queue)){
      LOG(ERROR) << "cannot verify transaction inputs.";
      return;
    }

    if(!verify_outputs_->Submit(queue)){
      LOG(ERROR) << "cannot verify transaction outputs.";
      return;
    }
  }
}