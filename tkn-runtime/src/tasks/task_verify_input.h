#ifndef TKN_TASK_VERIFY_INPUT_H
#define TKN_TASK_VERIFY_INPUT_H

#include "task/task.h"
#include "verifier/input_verifier.h"

namespace token{
  class VerifyTransactionInputsTask;
  class VerifyInputTask : public task::Task{
  private:
    Hash value_;
    InputVerifier verifier_;
  public:
    VerifyInputTask(VerifyTransactionInputsTask* parent, const Input& val, UnclaimedTransactionPool& pool);
    ~VerifyInputTask() override = default;

    std::string GetName() const override{
      return "VerifyInputTask";
    }

    void DoWork() override;
  };
}

#endif//TKN_TASK_VERIFY_INPUT_H