#ifndef TKN_TASK_VERIFY_TRANSACTION_OUTPUTS_H
#define TKN_TASK_VERIFY_TRANSACTION_OUTPUTS_H

#include "tasks/verify/task_verify_transaction_objects.h"

namespace token{
  class VerifyTransactionTask;
  class VerifyTransactionOutputsTask : public internal::VerifyTransactionObjectsTask<Output>,
                                       public OutputVisitor{
    friend class VerifyOutputTask;
  public:
    VerifyTransactionOutputsTask(VerifyTransactionTask* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val);
    ~VerifyTransactionOutputsTask() override = default;

    bool Visit(const OutputPtr& val) override;

    DECLARE_TASK_TYPE(VerifyTransactionOutputs);
  };
}

#endif//TKN_TASK_VERIFY_TRANSACTION_OUTPUTS_H