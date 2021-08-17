#ifndef TKN_TASK_VERIFY_TRANSACTION_INPUTS_H
#define TKN_TASK_VERIFY_TRANSACTION_INPUTS_H

#include "tasks/task_verify_transaction_objects.h"

namespace token{
  class VerifyTransactionTask;
  class VerifyTransactionInputsTask : public internal::VerifyTransactionObjectsTask<Input>,
                                      public InputVisitor{
    friend class VerifyInputTask;
  public:
    VerifyTransactionInputsTask(VerifyTransactionTask* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val);
    ~VerifyTransactionInputsTask() override = default;

    std::string GetName() const override{
      return "VerifyTransactionInputsTask()";
    }

    bool Visit(const Input& val) override;

    void DoWork() override{
      DVLOG(1) << "verifying transaction " << hash() << " inputs....";
      if(!transaction_->VisitInputs(this)){
        LOG(ERROR) << "cannot visit transaction inputs.";
        return;
      }
    }
  };
}

#endif//TKN_TASK_VERIFY_TRANSACTION_INPUTS_H