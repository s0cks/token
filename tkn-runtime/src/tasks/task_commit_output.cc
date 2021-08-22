#include "tasks/task_commit_output.h"
#include "tasks/task_commit_transaction_outputs.h"

namespace token{
  CommitOutputTask::CommitOutputTask(CommitTransactionOutputsTask* parent, internal::WriteBatchList& batches, OutputPtr val):
    task::Task(parent),
    value_(std::move(val)),
    batch_(batches.CreateNewBatch()){
  }

  void CommitOutputTask::DoWork(){
    internal::proto::UnclaimedTransaction raw;
    auto utxo = UnclaimedTransaction::NewInstance(raw);
    auto hash = utxo->hash();
    DLOG(INFO) << "creating unclaimed transaction: " << hash;
    batch_->Put(hash, utxo);
  }
}