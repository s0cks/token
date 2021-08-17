#include "tasks/task_verify_output.h"
#include "tasks/task_verify_transaction.h"

namespace token{
  VerifyOutputTask::VerifyOutputTask(VerifyTransactionOutputsTask* parent, const Output& val, UnclaimedTransactionPool& pool):
    task::Task(parent),
    value_(val),
    verifier_(pool){
  }

  void VerifyOutputTask::DoWork(){
    DLOG(INFO) << "verifying output " << value_ << "....";
    auto parent = ((VerifyTransactionOutputsTask*)GetParent());
    parent->processed_ += 1;
    parent->valid_ += 1;
    DLOG(INFO) << value_ << " is valid.";
  }
}