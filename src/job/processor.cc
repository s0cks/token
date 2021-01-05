#include "job/scheduler.h"
#include "job/processor.h"

namespace Token{
  JobResult ProcessBlockJob::DoWork(){
    if(!GetBlock()->Accept(this))
      return Failed("Cannot visit the block transactions.");

    for(auto it = hash_lists_.begin();
          it != hash_lists_.end();
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

      batch_.Delete(key);
      batch_.Put(key, value);
    }

    LOG(INFO) << "writing changes....";
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
    ProcessTransactionJob* job = new ProcessTransactionJob(this, tx);
    worker->Submit(job);
    worker->Wait(job);
    return true;
  }

  JobResult ProcessTransactionJob::DoWork(){
    JobWorker* worker = JobScheduler::GetThreadWorker();
    ProcessTransactionInputsJob* process_inputs = new ProcessTransactionInputsJob(this);
    worker->Submit(process_inputs);
    worker->Wait(process_inputs);

    ProcessTransactionOutputsJob* process_outputs = new ProcessTransactionOutputsJob(this);
    worker->Submit(process_outputs);
    worker->Wait(process_outputs);
    return Success("done.");
  }

  JobResult ProcessTransactionInputsJob::DoWork(){
    JobWorker* worker = JobScheduler::GetThreadWorker();

    TransactionPtr tx = GetTransaction();
    InputList& inputs = tx->inputs();
    auto start = inputs.begin();
    auto end = inputs.end();
    while(start != end){
      auto next = std::distance(start, end) > ProcessInputListJob::kMaxNumberOfInputs
                  ? start + ProcessInputListJob::kMaxNumberOfInputs
                  : end;

      InputList chunk(start, next);
      ProcessInputListJob* job = new ProcessInputListJob(this, chunk);
      if(!worker->Submit(job))
        return Failed("Cannot schedule ProcessInputListJob()");
      start = next;
    }

    return Success("done.");
  }

  JobResult ProcessTransactionOutputsJob::DoWork(){
    JobWorker* worker = JobScheduler::GetThreadWorker();
    TransactionPtr tx = GetTransaction();

    int64_t nworker = 0;
    OutputList& outputs = tx->outputs();
    auto start = outputs.begin();
    auto end = outputs.end();
    while(start != end){
      auto next = std::distance(start, end) > ProcessOutputListJob::kMaxNumberOfOutputs
                  ? start + ProcessOutputListJob::kMaxNumberOfOutputs
                  : end;

      OutputList chunk(start, next);
      ProcessOutputListJob* job = new ProcessOutputListJob(this, nworker++, chunk);
      if(!worker->Submit(job))
        return Failed("Cannot schedule ProcessOutputListJob()");

      start = next;
    }

    return Success("done.");
  }
}