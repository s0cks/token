#include "wallet.h"
#include "job/scheduler.h"
#include "job/processor.h"

namespace Token{
  JobResult ProcessBlockJob::DoWork(){
    if(!GetBlock()->Accept(this)){
      return Failed("Cannot visit the block transactions.");
    }

    for(auto& it : wallets_){
      const User& user = it.first;
      Wallet& wallet = it.second;
      WalletManager::GetWallet(user, wallet);
      LOG(INFO) << user << " now has " << wallet.size() << " unclaimed transactions.";
      RemoveObject(batch_wallet_, user);
      PutObject(batch_wallet_, user, wallet);
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
    RemoveObject(batch_pool_, tx->GetHash(), Object::Type::kTransaction);
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
      if(!worker->Submit(job)){
        return Failed("Cannot schedule ProcessInputListJob()");
      }
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
      if(!worker->Submit(job)){
        return Failed("Cannot schedule ProcessOutputListJob()");
      }

      start = next;
    }

    return Success("done.");
  }
}