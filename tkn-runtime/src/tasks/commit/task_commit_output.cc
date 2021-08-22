#include "tasks/commit/task_commit_output.h"
#include "tasks/commit/task_commit_transaction_outputs.h"

namespace token{
  CommitOutputTask::CommitOutputTask(CommitTransactionOutputsTask* parent, internal::WriteBatchList& batches, const TransactionReference& source, OutputPtr val):
    task::Task(parent),
    value_(std::move(val)),
    batch_(batches.CreateNewBatch()),
    source_(source){
  }

  void CommitOutputTask::DoWork(){
    auto utxo = UnclaimedTransaction::NewInstance(Clock::now(), source_, value_->user(), value_->product());
    auto hash = utxo->hash();
    DLOG(INFO) << "creating unclaimed transaction: " << hash;
    batch_->Put(hash, utxo);
  }
}