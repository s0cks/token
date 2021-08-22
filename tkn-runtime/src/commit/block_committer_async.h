#ifndef TKN_ASYNC_BLOCK_COMMITTER_H
#define TKN_ASYNC_BLOCK_COMMITTER_H

#include "block_committer.h"
#include "tasks/task_commit_transaction.h"

namespace token{
  namespace async{
    class BlockCommitter : public internal::BlockCommitterBase,
                           public IndexedTransactionVisitor{
    private:
      task::TaskEngine& engine_;
      task::TaskQueue& queue_;
      internal::WriteBatchList batches_;
      std::vector<std::shared_ptr<CommitTransactionTask>> tasks_;

      inline std::shared_ptr<CommitTransactionTask>
      CreateCommitTransactionTask(const IndexedTransactionPtr& val){
        auto task = std::make_shared<CommitTransactionTask>(&engine_, batches_, val);
        tasks_.push_back(task);
        return task;
      }
    public:
      BlockCommitter(task::TaskEngine& engine, task::TaskQueue& queue, const BlockPtr& blk, UnclaimedTransactionPool& utxos):
        internal::BlockCommitterBase(blk, utxos),
        IndexedTransactionVisitor(),
        engine_(engine),
        queue_(queue),
        tasks_(),
        batches_(){
        tasks_.reserve(blk->GetNumberOfTransactions());
      }
      ~BlockCommitter() override = default;

      task::TaskEngine& GetTaskEngine() const{
        return engine_;
      }

      task::TaskQueue& GetTaskQueue() const{
        return queue_;
      }

      bool Visit(const IndexedTransactionPtr& val) override;
      bool Commit() override;
      void PrintStats();
    };
  }
}

#endif//TKN_ASYNC_BLOCK_COMMITTER_H