#ifndef TKN_TASK_VERIFY_TRANSACTION_INPUTS_H
#define TKN_TASK_VERIFY_TRANSACTION_INPUTS_H

#include "tasks/verify/task_verify_transaction_objects.h"

namespace token{
  class VerifyTransactionTask;
  class VerifyTransactionInputsTask : public internal::VerifyTransactionObjectsTask<Input>,
                                      public InputVisitor{
    friend class VerifyInputTask;
  public:
    VerifyTransactionInputsTask(VerifyTransactionTask* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val);
    ~VerifyTransactionInputsTask() override = default;
    bool Visit(const InputPtr& val) override;
    DECLARE_TASK_TYPE(VerifyTransactionInputs);
  };
}

#endif//TKN_TASK_VERIFY_TRANSACTION_INPUTS_H