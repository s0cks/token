#ifndef TKN_BLOCK_VERIFIER_ASYNC_H
#define TKN_BLOCK_VERIFIER_ASYNC_H

#include "task/task_engine.h"
#include "verifier/block_verifier.h"
#include "tasks/verify/task_verify_transaction.h"

namespace token{
  namespace async{
    class BlockVerifier : public internal::BlockVerifierBase,
                          public IndexedTransactionVisitor{
    private:
      task::TaskEngine& engine_;
      task::TaskQueue& queue_;
      std::vector<std::shared_ptr<VerifyTransactionTask>> verify_transactions_;

      inline std::shared_ptr<VerifyTransactionTask>
      CreateVerifyTransactionTask(const IndexedTransactionPtr& val){
        auto task = std::make_shared<VerifyTransactionTask>(&engine_, utxos(), val);
        verify_transactions_.push_back(task);
        return task;
      }
    public:
      BlockVerifier(task::TaskEngine& engine, task::TaskQueue& queue, const BlockPtr& blk, UnclaimedTransactionPool& pool):
        internal::BlockVerifierBase(blk, pool),
        IndexedTransactionVisitor(),
        engine_(engine),
        queue_(queue),
        verify_transactions_(){
        verify_transactions_.reserve(blk->GetNumberOfTransactions());
      }
      ~BlockVerifier() override = default;

      task::TaskEngine& GetTaskEngine() const{
        return engine_;
      }

      task::TaskQueue& GetTaskQueue() const{
        return queue_;
      }

      bool Visit(const IndexedTransactionPtr& val) override;
      bool Verify() override;

      void PrintStats();
    };
  }
}

#endif//TKN_BLOCK_VERIFIER_ASYNC_H