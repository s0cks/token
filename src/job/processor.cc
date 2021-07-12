/*
#include "wallet.h"
#include "job/scheduler.h"
#include "job/processor.h"

namespace token{
  JobResult ProcessBlockJob::DoWork(){
    if(!GetBlock()->Accept(this))
      return Failed("Cannot visit the block transactions.");
    return Success("Finished.");
  }

  bool ProcessBlockJob::Visit(const IndexedTransactionPtr& tx){
    JobQueue* queue = JobScheduler::GetThreadQueue();
    auto job = new ProcessTransactionJob(this, tx);
    queue->Push(job);
    //TODO: RemoveObject(batch_, tx->GetHash(), Type::kTransaction);
    return true;
  }

  JobResult ProcessTransactionJob::DoWork(){
    JobQueue* queue = JobScheduler::GetThreadQueue();
    auto process_inputs = new ProcessTransactionInputsJob(this);
    queue->Push(process_inputs);

    auto process_outputs = new ProcessTransactionOutputsJob(this);
    queue->Push(process_outputs);
    return Success("done.");
  }

  JobResult ProcessTransactionInputsJob::DoWork(){
    JobQueue* queue = JobScheduler::GetThreadQueue();

    IndexedTransactionPtr tx = GetTransaction();
    InputList& inputs = tx->inputs();
    auto start = inputs.begin();
    auto end = inputs.end();
    while(start != end){
      auto next = std::distance(start, end) > ProcessInputListJob::kMaxNumberOfInputs
                  ? start + ProcessInputListJob::kMaxNumberOfInputs
                  : end;

      InputList chunk(start, next);
      auto job = new ProcessInputListJob(this, chunk);
      if(!queue->Push(job)){
        return Failed("Cannot schedule ProcessInputListJob()");
      }
      start = next;
    }

    return Success("done.");
  }

  JobResult ProcessTransactionOutputsJob::DoWork(){
    JobQueue* queue = JobScheduler::GetThreadQueue();
    IndexedTransactionPtr tx = GetTransaction();

    int64_t nworker = 0;
    OutputList& outputs = tx->outputs();
    auto start = outputs.begin();
    auto end = outputs.end();
    while(start != end){
      auto next = std::distance(start, end) > ProcessOutputListJob::kMaxNumberOfOutputs
                  ? start + ProcessOutputListJob::kMaxNumberOfOutputs
                  : end;

      OutputList chunk(start, next);
      auto job = new ProcessOutputListJob(this, nworker++, chunk);
      if(!queue->Push(job)){
        return Failed("Cannot schedule ProcessOutputListJob()");
      }

      start = next;
    }

    return Success("done.");
  }

  JobResult ProcessOutputListJob::DoWork(){
    for(auto& it : outputs_){
      UnclaimedTransactionPtr val = CreateUnclaimedTransaction(it);
      Hash hash = val->hash();
      PutUnclaimedTransaction(hash, val);
      Track(val->GetUser(), hash);
    }

    if(!Commit()){
      LOG_JOB(ERROR, this) << "cannot commit ~" << GetCurrentBatchSize() << "b of changes to object pool.";
      return Failed("Cannot Commit Changes.");
    }

    ((ProcessTransactionOutputsJob*)GetParent())->Append(wallets_);
    return Success("done.");
  }
}*/
