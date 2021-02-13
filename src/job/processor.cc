#include "wallet.h"
#include "job/scheduler.h"
#include "job/processor.h"

namespace token{
#define JOB_LOG(LevelName) \
  LOG(LevelName) << "[" << GetName() << "] "

  leveldb::Status ProcessBlockJob::CommitPoolChanges(){
    if(batch_pool_.ApproximateSize() == 0)
      return leveldb::Status::IOError("the object pool batch write is ~0b in size");

#ifdef TOKEN_DEBUG
    JOB_LOG(INFO) << "committing ~" << batch_pool_.ApproximateSize() << "b of changes.";
#endif//TOKEN_DEBUG
    return ObjectPool::Write(&batch_pool_);
  }

  JobResult ProcessBlockJob::DoWork(){
    if(!GetBlock()->Accept(this)){
      return Failed("Cannot visit the block transactions.");
    }
    return Success("Finished.");
  }

  bool ProcessBlockJob::Visit(const TransactionPtr& tx){
    JobQueue* queue = JobScheduler::GetThreadQueue();
    ProcessTransactionJob* job = new ProcessTransactionJob(this, tx);
    queue->Push(job);
    RemoveObject(batch_pool_, tx->GetHash(), Type::kTransaction);
    return true;
  }

  JobResult ProcessTransactionJob::DoWork(){
    JobQueue* queue = JobScheduler::GetThreadQueue();

#ifdef TOKEN_DEBUG
    JOB_LOG(INFO) << "processing transaction: " << transaction_->ToString();
#else
    JOB_LOG(INFO) << "processing transaction: " << hash_;
#endif//TOKEN_DEBUG

    ProcessTransactionInputsJob* process_inputs = new ProcessTransactionInputsJob(this);
    queue->Push(process_inputs);

    ProcessTransactionOutputsJob* process_outputs = new ProcessTransactionOutputsJob(this);
    queue->Push(process_outputs);
    return Success("done.");
  }

  JobResult ProcessTransactionInputsJob::DoWork(){
    JobQueue* queue = JobScheduler::GetThreadQueue();

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
      if(!queue->Push(job)){
        return Failed("Cannot schedule ProcessInputListJob()");
      }
      start = next;
    }

    return Success("done.");
  }

  JobResult ProcessTransactionOutputsJob::DoWork(){
    JobQueue* queue = JobScheduler::GetThreadQueue();
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
      if(!queue->Push(job)){
        return Failed("Cannot schedule ProcessOutputListJob()");
      }

      start = next;
    }

    return Success("done.");
  }

  JobResult ProcessOutputListJob::DoWork(){
#ifdef TOKEN_DEBUG
    JOB_LOG(INFO) << "processing " << outputs_.size() << " outputs....";
#endif//TOKEN_DEBUG

    for(auto& it : outputs_){
      UnclaimedTransactionPtr val = CreateUnclaimedTransaction(it);
      Hash hash = val->GetHash();
      PutObject(batch_, hash, val);
      Track(val->GetUser(), hash);
    }

    JOB_LOG(INFO) << "wallets (" << wallets_.size() << "):";
    for(auto& it : wallets_){
      JOB_LOG(INFO) << " - " << it.first << " (" << it.second.size() << ") " << " := " << it.second;
    }

#ifdef TOKEN_DEBUG
    JOB_LOG(INFO) << "appending batch of ~" << batch_.ApproximateSize() << "b";
#endif//TOKEN_DEBUG
    ((ProcessTransactionOutputsJob*) GetParent())->Append(batch_);
    return Success("done.");
  }
}