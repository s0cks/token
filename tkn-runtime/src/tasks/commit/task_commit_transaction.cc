#include "task/task_engine.h"
#include "tasks/commit/task_commit_transaction.h"

namespace token{
  void CommitTransactionTask::DoWork(){
    auto& queue = GetEngine()->GetCurrentWorker()->GetTaskQueue();
    if(!inputs_->Submit(queue)){
      LOG(ERROR) << "cannot commit transaction inputs.";
      return;
    }

    if(!outputs_->Submit(queue)){
      LOG(ERROR) << "cannot commit transaction outputs.";
      return;
    }
  }

#ifdef TOKEN_DEBUG
  void CommitTransactionTask::PrintStats(){
    NOT_IMPLEMENTED(ERROR);//TODO: implement
  }
#endif//TOKEN_DEBUG

  std::string CommitTransactionTask::ToString() const{
    std::stringstream ss;
    ss << "CommitTransactionTask(";
    ss << "value=" << transaction_->hash();
    ss << ")";
    return ss.str();
  }
}