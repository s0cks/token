#ifndef TKN_TASK_VERIFY_TRANSACTION_H
#define TKN_TASK_VERIFY_TRANSACTION_H

#include "tasks/task_verify_transaction_inputs.h"
#include "tasks/task_verify_transaction_outputs.h"

namespace token{
  class VerifyTransactionTask : public task::Task{
    friend class VerifyInputTask;
    friend class VerifyOutputTask;
  private:
    UnclaimedTransactionPool& pool_;
    IndexedTransactionPtr transaction_;
    std::shared_ptr<VerifyTransactionInputsTask> verify_inputs_;
    std::shared_ptr<VerifyTransactionOutputsTask> verify_outputs_;

    inline UnclaimedTransactionPool&
    pool() const{
      return pool_;
    }
  public:
    explicit VerifyTransactionTask(task::TaskEngine* engine, UnclaimedTransactionPool& pool, const IndexedTransactionPtr& tx):
        task::Task(engine),
        pool_(pool),
        transaction_(IndexedTransaction::CopyFrom(tx)),
        verify_inputs_(std::make_shared<VerifyTransactionInputsTask>(this, pool, tx)),
        verify_outputs_(std::make_shared<VerifyTransactionOutputsTask>(this, pool, tx)){}
    ~VerifyTransactionTask() override = default;

    std::string GetName() const override{
      return "VerifyTransactionTask";
    }

    std::shared_ptr<VerifyTransactionInputsTask>
    GetVerifyInputs() const{
      return verify_inputs_;
    }

    std::shared_ptr<VerifyTransactionOutputsTask>
    GetVerifyOutputs() const{
      return verify_outputs_;
    }

    void DoWork() override;
  };
}

#endif//TKN_TASK_VERIFY_TRANSACTION_H