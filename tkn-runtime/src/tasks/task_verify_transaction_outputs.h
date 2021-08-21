#ifndef TKN_TASK_VERIFY_TRANSACTION_OUTPUTS_H
#define TKN_TASK_VERIFY_TRANSACTION_OUTPUTS_H

#include "tasks/task_verify_transaction_objects.h"

namespace token{
  class VerifyTransactionTask;
  class VerifyTransactionOutputsTask : public internal::VerifyTransactionObjectsTask<Output>,
                                       public OutputVisitor{
    friend class VerifyOutputTask;
  public:
    VerifyTransactionOutputsTask(VerifyTransactionTask* parent, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val);
    ~VerifyTransactionOutputsTask() override = default;

    std::string GetName() const override{
      return "VerifyTransactionOutputsTask()";
    }

    bool Visit(const OutputPtr& val) override;

    void DoWork() override{
      DVLOG(1) << "verifying transaction " << hash() << " outputs....";
      if(!transaction_->VisitOutputs(this)){
        LOG(ERROR) << "cannot visit transaction inputs.";
        return;
      }
    }
  };
}

#endif//TKN_TASK_VERIFY_TRANSACTION_OUTPUTS_H