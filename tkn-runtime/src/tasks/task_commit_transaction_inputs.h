#ifndef TKN_TASK_COMMIT_TRANSACTION_INPUTS_H
#define TKN_TASK_COMMIT_TRANSACTION_INPUTS_H

#include "tasks/task_commit_transaction_objects.h"

namespace token{
  class CommitTransactionInputsTask;
  typedef std::shared_ptr<CommitTransactionInputsTask> CommitTransactionInputsTaskPtr;

  class CommitTransactionTask;
  class CommitTransactionInputsTask : public internal::CommitTransactionObjectsTask<Input>,
                                      public InputVisitor{
  public:
    CommitTransactionInputsTask(CommitTransactionTask* parent, internal::WriteBatchList& batches, const IndexedTransactionPtr& val);
    ~CommitTransactionInputsTask() override = default;

    std::string GetName() const override{
      return "CommitTransactionInputsTask()";
    }

    bool Visit(const InputPtr& val) override;

    void DoWork() override{
      DVLOG(2) << "committing transaction " << hash() << " inputs....";
      if(!transaction_->VisitInputs(this)){
        LOG(ERROR) << "cannot visit transaction inputs.";
        return;
      }
    }

    static inline CommitTransactionInputsTaskPtr
    NewInstance(CommitTransactionTask* parent, internal::WriteBatchList& batches, const IndexedTransactionPtr& val){
      return std::make_shared<CommitTransactionInputsTask>(parent, batches, val);
    }
  };
}

#endif//TKN_TASK_COMMIT_TRANSACTION_INPUTS_H