#include "pool.h"
#include "job/verifier.h"
#include "job/scheduler.h"

namespace token{
  JobResult VerifyBlockJob::DoWork(){
    JobQueue* queue = JobScheduler::GetThreadQueue();
    std::vector<VerifyTransactionJob*> jobs;

    LOG(INFO) << "verifying " << block_->GetNumberOfTransactions() << " transactions....";
    for(auto& it : block_->transactions()){
      Hash hash = it->GetHash();
      LOG(INFO) << "visiting transaction " << hash;
      VerifyTransactionJob* verify_tx = new VerifyTransactionJob(this, it);
      queue->Push(verify_tx);

      jobs.push_back(verify_tx);
    }

    for(auto& it : jobs){
      while(!it->IsFinished()); // spin
      JobResult& result = it->GetResult();
      if(!result.IsSuccessful()){
        return result;
      }
    }
    return Success("done.");
  }

  JobResult VerifyTransactionJob::DoWork(){
    JobQueue* queue = JobScheduler::GetThreadQueue();
    VerifyTransactionInputsJob* verify_inputs = new VerifyTransactionInputsJob(this, transaction_->inputs());
    queue->Push(verify_inputs);
    while(!verify_inputs->IsFinished()); //spin
    if(!verify_inputs->GetResult().IsSuccessful()){
      return verify_inputs->GetResult();
    }

    VerifyTransactionOutputsJob* verify_outputs = new VerifyTransactionOutputsJob(this, transaction_->outputs());
    queue->Push(verify_outputs);
    while(!verify_outputs->IsFinished()); //spin
    if(!verify_outputs->GetResult().IsSuccessful()){
      return verify_outputs->GetResult();
    }

    return Success("done.");
  }

  JobResult VerifyTransactionInputsJob::DoWork(){
    JobQueue* queue = JobScheduler::GetThreadQueue();
    std::vector<VerifyInputListJob*> jobs;

    TransactionPtr tx = GetTransaction();
    Hash hash = tx->GetHash();

    InputList& inputs = tx->inputs();
    auto start = inputs.begin();
    auto end = inputs.end();
    while(start != end){
      auto next = std::distance(start, end) > VerifyInputListJob::kMaxNumberOfInputs
                  ? start + VerifyInputListJob::kMaxNumberOfInputs
                  : end;

      InputList chunk(start, next);
      VerifyInputListJob* job = new VerifyInputListJob(this, chunk);
      if(!queue->Push(job)){
        return Failed("Cannot schedule VerifyInputListJob()");
      }
      jobs.push_back(job);
      start = next;
    }

    for(auto& it : jobs){
      while(!it->IsFinished()); //spin

      JobResult& result = it->GetResult();
      if(!result.IsSuccessful()){
        InputList& invalid = it->GetInvalid();

        std::stringstream ss;
        ss << "Transaction " << hash << " has " << invalid.size() << " invalid inputs:" << std::endl;
        for(auto in : invalid)
          ss << " - " << in << std::endl;

        //TODO: cleanup remaining jobs
        return Failed(ss);
      }

      LOG(INFO) << "validated " << it->GetValid().size() << " inputs.";
      delete it;
    }
    return Success("done.");
  }

  //TODO: refactor
  JobResult VerifyInputListJob::DoWork(){
    ObjectPoolPtr pool = ObjectPool::GetInstance();
    for(auto& it : inputs_){
      UnclaimedTransactionPtr utxo = pool->FindUnclaimedTransaction(it);
      if(!utxo){
        LOG(WARNING) << "no unclaimed transaction found for: " << it;
        invalid_.push_back(it);
        continue;
      }
      valid_.push_back(it);
    }

    return invalid_.empty()
           ? Success("done.")
           : Failed("Not all transactions are IsValid.");
  }

  JobResult VerifyTransactionOutputsJob::DoWork(){
    JobQueue* queue = JobScheduler::GetThreadQueue();

    TransactionPtr tx = GetTransaction();
    OutputList& outputs = tx->outputs();
    auto start = outputs.begin();
    auto end = outputs.end();
    while(start != end){
      auto next = std::distance(start, end) > VerifyOutputListJob::kMaxNumberOfOutputs
                  ? start + VerifyOutputListJob::kMaxNumberOfOutputs
                  : end;

      OutputList chunk(start, next);
      VerifyOutputListJob* job = new VerifyOutputListJob(this, chunk);
      if(!queue->Push(job)){
        return Failed("Cannot schedule VerifyInputListJob()");
      }
      start = next;
    }

    return Success("done.");
  }

  JobResult VerifyOutputListJob::DoWork(){
    //TODO: implement VerifyOutputList::DoWork()
    return Success("done.");
  }
}