#ifndef TKN_TASK_VERIFY_OUTPUT_H
#define TKN_TASK_VERIFY_OUTPUT_H

#include "task/task.h"
#include "verifier/verifier_output.h"

namespace token{
  class VerifyTransactionOutputsTask;
  class VerifyOutputTask : public task::Task{
  private:
    Output value_;
    OutputVerifier verifier_;
  public:
    VerifyOutputTask(VerifyTransactionOutputsTask* parent, const Output& val, UnclaimedTransactionPool& pool);
    ~VerifyOutputTask() override = default;

    std::string GetName() const override{
      return "VerifyOutputTask";
    }

    void DoWork() override;
  };
}

#endif//TKN_TASK_VERIFY_OUTPUT_H