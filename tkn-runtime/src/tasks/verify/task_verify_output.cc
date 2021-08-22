#include "tasks/verify/task_verify_output.h"
#include "tasks/verify/task_verify_transaction.h"

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

  std::string VerifyOutputTask::ToString() const{
    std::stringstream ss;
    ss << "VerifyOutputTask(";
    ss << "value=" << value_->ToString();
    ss << ")";
    return ss.str();
  }
}