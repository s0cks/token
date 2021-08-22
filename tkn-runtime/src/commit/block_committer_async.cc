#include "block.h"
#include "block_committer_async.h"

namespace token{
  namespace async{
    bool BlockCommitter::Visit(const IndexedTransactionPtr& val){
      auto start = Clock::now();
      auto tx_hash = val->hash();
      auto& queue = GetTaskQueue();
      auto task = CreateCommitTransactionTask(val);
      if(!task->Submit(queue)){
        LOG(ERROR) << "cannot commit transaction " << tx_hash << ".";
        return false;
      }

      DLOG(INFO) << "waiting for CommitTransactionTask() to finish....";
      while(!task->IsFinished());//spin
      DLOG(INFO) << "CommitTransactionTask() has finished, took=" << GetElapsedTimeMilliseconds(start) << "ms.";
      return true;
    }

    bool BlockCommitter::Commit(){
      auto start = Clock::now();

      if(!block_->VisitTransactions(this)){
        LOG(ERROR) << "cannot visit Block transactions.";
        return false;
      }

      sleep(12);//TODO: remove this should not exist

      internal::WriteBatch batch;
      if(!batches_.Compile(batch)){
        LOG(ERROR) << "cannot compile batches.";
        return false;
      }

      if(!utxos().Commit(&batch)){
        LOG(ERROR) << "cannot commit batch.";
        return false;
      }

      DLOG(INFO) << "Commit() has finished, took=" << GetElapsedTimeMilliseconds(start) << "ms.";
      return true;
    }

    void BlockCommitter::PrintStats(){
      LOG(INFO) << "CommitBlockTask() stats: ";
      LOG(INFO) << "\tbatches: " << batches_.GetNumberOfBatches();
    }
  }
}