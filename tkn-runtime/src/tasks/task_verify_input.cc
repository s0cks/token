#include "tasks/task_verify_transaction.h"
#include "tasks/task_verify_input.h"

namespace token{
  VerifyInputTask::VerifyInputTask(VerifyTransactionInputsTask* parent, const Input& val, UnclaimedTransactionPool& pool):
    task::Task(parent),
    verifier_(pool),
    value_(val.hash()){}

  void VerifyInputTask::DoWork(){
    DLOG(INFO) << "verifying input " << value_ << "....";
    auto parent = ((VerifyTransactionInputsTask*)GetParent());
    parent->processed_ += 1;
    if(!verifier_.IsValid(value_)){
      parent->invalid_ += 1;
      DLOG(ERROR) << value_ << " is invalid.";
      return;
    }
    parent->valid_ += 1;
    DLOG(INFO) << value_ << " is valid.";
  }
}