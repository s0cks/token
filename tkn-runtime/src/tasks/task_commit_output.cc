#include "tasks/task_commit_output.h"
#include "tasks/task_commit_transaction_outputs.h"

namespace token{
  CommitOutputTask::CommitOutputTask(CommitTransactionOutputsTask* parent, OutputPtr val):
    task::Task(parent),
    value_(std::move(val)){
  }

  void CommitOutputTask::DoWork(){

  }
}