#ifndef TKN_TASK_COMMIT_TRANSACTION_OUTPUTS_H
#define TKN_TASK_COMMIT_TRANSACTION_OUTPUTS_H

#include "tasks/task_commit_transaction_objects.h"

namespace token{
  class CommitTransactionTask;
  class CommitTransactionOutputsTask : public internal::CommitTransactionObjectsTask<Output>,
                                       public OutputVisitor{
  public:
    CommitTransactionOutputsTask(CommitTransactionTask* parent, internal::WriteBatchList& batches, const IndexedTransactionPtr& val);
    ~CommitTransactionOutputsTask() override = default;

    std::string GetName() const override{
      return "CommitTransactionOutputsTask()";
    }

    bool Visit(const OutputPtr& val) override;

    void DoWork() override{
      DVLOG(2) << "committing transaction " << hash() << " outputs....";
      if(!transaction_->VisitOutputs(this)){
        LOG(ERROR) << "cannot visit transaction outputs.";
        return;
      }
    }

    static inline std::shared_ptr<CommitTransactionOutputsTask>
    NewInstance(CommitTransactionTask* parent, internal::WriteBatchList& batches, const IndexedTransactionPtr& val){
      return std::make_shared<CommitTransactionOutputsTask>(parent, batches, val);
    }
  };
}

#endif//TKN_TASK_COMMIT_TRANSACTION_OUTPUTS_H