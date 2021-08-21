#include "block_verifier_async.h"

namespace token{
  namespace async{
    bool BlockVerifier::Visit(const IndexedTransactionPtr& val){
      auto start = Clock::now();

      auto tx_hash = val->hash();
      DVLOG(2) << "visiting transaction " << tx_hash << "....";

      auto& queue = GetTaskQueue();
      auto task = CreateVerifyTransactionTask(val);
      if(!task->Submit(queue)){
        LOG(ERROR) << "cannot verify transaction " << tx_hash << ".";
        return false;
      }

      DVLOG(1) << "waiting for task to finish.";
      while(!task->IsFinished());//spin
      DVLOG(1) << "task has finished, took=" << GetElapsedTimeMilliseconds(start) << "ms.";
      return true;
    }

    bool BlockVerifier::Verify(){
      return block()->VisitTransactions(this);
    }

    void BlockVerifier::PrintStats(){
      for(auto& task : verify_transactions_){
        auto hash = task->GetTransaction()->hash();
        DLOG(INFO) << "transaction " << hash << " results:";
        DLOG(INFO) << "\tinputs: " << task->GetVerifyInputs()->GetStats();
        DLOG(INFO) << "\toutputs: " << task->GetVerifyOutputs()->GetStats();
      }
    }
  }
}