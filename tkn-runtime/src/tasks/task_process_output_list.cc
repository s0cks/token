#include <glog/logging.h>

#include "key.h"
#include "transaction_unclaimed.h"
#include "tasks/task_process_transaction.h"
#include "tasks/task_process_output_list.h"

namespace token{
  namespace task{
    ProcessOutputListTask::ProcessOutputListTask(ProcessTransactionTask* parent, ObjectPool& pool, const Hash& hash, const int64_t& index, std::vector<Output>& outputs):
      task::Task(parent),
      pool_(pool),
      batch_(),
      counter_(parent->processed_outputs_),
      outputs_(outputs),
      transaction_(hash),
      index_(index){}

    void ProcessOutputListTask::DoWork(){
      DVLOG(2) << "processing output list of size " << outputs_.size() << "....";
      for(auto& it : outputs_){
        UnclaimedTransaction::Builder builder;
        builder.SetTransactionHash(transaction_);
        builder.SetTransactionIndex(index_++);
        builder.SetUser(it.user().ToString());
        builder.SetProduct(it.product().ToString());

        auto utxo = builder.Build();
        auto hash = utxo->hash();
        batch_.PutUnclaimedTransaction(hash, utxo);
        counter_ += 1;
      }

      leveldb::Status status;
      if(!(status = pool_.Commit(batch_.batch())).ok())
        LOG(FATAL) << "cannot write batch to pool: " << status.ToString();
    }
  }
}