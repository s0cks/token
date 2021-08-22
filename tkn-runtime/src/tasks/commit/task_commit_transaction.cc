#include "task/task_engine.h"
#include "tasks/commit/task_commit_transaction.h"

namespace token{
  void CommitTransactionTask::DoWork(){
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    if(!commit_inputs_->Submit(queue)){
      LOG(ERROR) << "cannot commit transaction inputs.";
      return;
    }

    if(!commit_outputs_->Submit(queue)){
      LOG(ERROR) << "cannot commit transaction outputs.";
      return;
    }
  }
}