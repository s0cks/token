#include "tasks/task_commit_input.h"
#include "tasks/task_commit_transaction_inputs.h"

namespace token{
  CommitInputTask::CommitInputTask(CommitTransactionInputsTask* parent, const InputPtr& val):
    task::Task(parent),
    value_(val){
  }

  void CommitInputTask::DoWork(){

  }
}