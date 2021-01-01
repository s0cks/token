#include "job/verifier.h"
#include "job/scheduler.h"

namespace Token{
  class VerifyInputListJob : public VerifierJob{
   private:
    const InputList &inputs_;
   protected:
    JobResult DoWork(){
//TODO: verify input
//            for(auto& it : inputs_)
      return Success("done.");
    }
   public:
    VerifyInputListJob(VerifyTransactionJob *parent, const InputList &inputs):
      VerifierJob(parent, "VerifyInputList"),
      inputs_(inputs){}
    ~VerifyInputListJob() = default;
  };

  class VerifyOutputListJob : public VerifierJob{
   private:
    const OutputList &outputs_;
   protected:
    JobResult DoWork(){
//TODO: verify output
//            for(auto& it : outputs_)
      return Success("done.");
    }
   public:
    VerifyOutputListJob(VerifyTransactionJob *parent, const OutputList &outputs):
      VerifierJob(parent, "VerifyOutputList"),
      outputs_(outputs){}
    ~VerifyOutputListJob() = default;
  };

  VerifyTransactionJob::VerifyTransactionJob(VerifyBlockJob *parent, const TransactionPtr &tx):
    VerifierJob(parent, "VerifyTransaction"),
    transaction_(tx){}

  JobResult VerifyTransactionJob::DoWork(){
    JobWorker *worker = JobScheduler::GetThreadWorker();
    VerifyInputListJob *verify_inputs = new VerifyInputListJob(this, transaction_->inputs());
    worker->Submit(verify_inputs);

    VerifyOutputListJob *verify_outputs = new VerifyOutputListJob(this, transaction_->outputs());
    worker->Submit(verify_outputs);
    return Success("done.");
  }

  JobResult VerifyBlockJob::DoWork(){
    JobWorker *worker = JobScheduler::GetThreadWorker();
    for(auto &it : block_->transactions()){
      VerifyTransactionJob *verify_tx = new VerifyTransactionJob(this, it);
      worker->Submit(verify_tx);
    }
    return Success("done.");
  }
}