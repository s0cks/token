#include "job/scheduler.h"
#include "job/process_block.h"
#include "job/process_transaction.h"

namespace Token{
  JobResult ProcessBlockJob::DoWork(){
    if(!GetBlock()->Accept(this))
      return Failed("Cannot visit the block transactions.");

    for(auto it = hash_lists_->begin();
          it != hash_lists_->end();
          it++){
      const User& user = (*it).first;
      HashList& hashes = (*it).second;
      ObjectPool::GetUnclaimedTransactionsFor(user, hashes);

      LOG(INFO) << user << " now has " << hashes.size() << " unclaimed transactions.";

      std::string key = user.Get();

      int64_t val_size = GetBufferSize(hashes);
      uint8_t val_data[val_size];
      Encode(hashes, val_data, val_size);
      leveldb::Slice value((char*) val_data, val_size);

      GetBatch()->Delete(key);
      GetBatch()->Put(key, value);
    }

    leveldb::Status status;
    if(!(status = Write()).ok()){
      std::stringstream ss;
      ss << "Cannot write changes: " << status.ToString();
      return Failed(ss);
    }
    return Success("Finished.");
  }

  bool ProcessBlockJob::Visit(const TransactionPtr& tx){
    JobWorker* worker = JobScheduler::GetThreadWorker();
    LOG(INFO) << "submitting worker for: " << tx->GetHash();
    ProcessTransactionJob* job = new ProcessTransactionJob(this, tx);
    worker->Submit(job);
    worker->Wait(job);
    return true;
  }
}