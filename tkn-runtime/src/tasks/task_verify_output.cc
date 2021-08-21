#include "tasks/task_verify_output.h"

#include <utility>
#include "tasks/task_verify_transaction.h"

namespace token{
  VerifyOutputTask::VerifyOutputTask(VerifyTransactionOutputsTask* parent, OutputPtr val, UnclaimedTransactionPool& pool):
    task::Task(parent),
    value_(std::move(val)),
    verifier_(pool){
  }

  void VerifyOutputTask::DoWork(){
    DLOG(INFO) << "verifying output " << value_->ToString() << "....";
    auto parent = ((VerifyTransactionOutputsTask*)GetParent());
    parent->processed_ += 1;
    parent->valid_ += 1;
    DLOG(INFO) << value_->ToString() << " is valid.";
  }
}