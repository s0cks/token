#include "block_verifier_async.h"

namespace token{
  namespace async{
    bool BlockVerifier::Visit(const IndexedTransactionPtr& val){
      auto start = Clock::now();

      auto tx_hash = val->hash();
      DVLOG(2) << "visiting transaction " << tx_hash << "....";

      auto& queue = GetTaskQueue();
      auto task = std::make_shared<VerifyTransactionTask>(&engine_, utxos(), val);
      if(!task->Submit(queue)){
        LOG(ERROR) << "cannot verify transaction " << tx_hash << ".";
        return false;
      }

      DVLOG(1) << "waiting for task to finish.";
      while(!task->IsFinished());//spin
      DVLOG(1) << "task has finished, took=" << GetElapsedTimeMilliseconds(start) << "ms.";

      auto verify_inputs = task->GetVerifyInputs();
      auto verify_outputs = task->GetVerifyOutputs();
      DVLOG(1) << "transaction " << tx_hash << " results:";
      DVLOG(1) << "inputs: " << verify_inputs->GetStats();
      DVLOG(1) << "outputs: " << verify_outputs->GetStats();
      return true;
    }

    bool BlockVerifier::Verify(){
      return block()->VisitTransactions(this);
    }
  }
}