#ifndef TKN_TASK_VERIFY_OUTPUT_H
#define TKN_TASK_VERIFY_OUTPUT_H

#include "task/task.h"
#include "verifier/verifier_output.h"

namespace token{
  class VerifyTransactionOutputsTask;
  class VerifyOutputTask : public task::Task{
  private:
    OutputPtr value_;
    OutputVerifier verifier_;
  public:
    VerifyOutputTask(VerifyTransactionOutputsTask* parent, OutputPtr  val, UnclaimedTransactionPool& pool);
    ~VerifyOutputTask() override = default;
    DECLARE_TASK_TYPE(VerifyOutput);
  };
}

#endif//TKN_TASK_VERIFY_OUTPUT_H