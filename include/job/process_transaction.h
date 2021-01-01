#ifndef TOKEN_PROCESS_TRANSACTION_H
#define TOKEN_PROCESS_TRANSACTION_H

#include <leveldb/write_batch.h>
#include "job/job.h"
#include "job/process_block.h"
#include "utils/atomic_linked_list.h"

namespace Token{
  class ProcessTransactionJob : public WriteBatchJob{
   protected:
    TransactionPtr transaction_;

    JobResult DoWork();
   public:
    ProcessTransactionJob(ProcessBlockJob *parent, const TransactionPtr &tx):
      WriteBatchJob(parent, "ProcessTransaction"),
      transaction_(tx){}
    ~ProcessTransactionJob() = default;

    BlockPtr GetBlock() const{
      return ((ProcessBlockJob *) GetParent())->GetBlock();
    }

    bool ShouldClean() const{
      return ((ProcessBlockJob *) GetParent())->ShouldClean();
    }

    TransactionPtr GetTransaction() const{
      return transaction_;
    }

    void Track(const UserHashLists &hashes){
      return ((ProcessBlockJob *) GetParent())->Track(hashes);
    }

    void Append(leveldb::WriteBatch *batch){
      return ((ProcessBlockJob *) GetParent())->Append(batch);
    }
  };

  class ProcessTransactionInputsJob : public WriteBatchJob{
   protected:
    JobResult DoWork();
   public:
    ProcessTransactionInputsJob(ProcessTransactionJob *parent):
      WriteBatchJob(parent, "ProcessTransactionInputsJob"){}
    ~ProcessTransactionInputsJob() = default;

    TransactionPtr GetTransaction() const{
      return ((ProcessTransactionJob *) GetParent())->GetTransaction();
    }

    void Append(leveldb::WriteBatch *batch){
      return ((ProcessTransactionJob *) GetParent())->Append(batch);
    }
  };

  class ProcessTransactionOutputsJob : public WriteBatchJob{
   protected:
    JobResult DoWork();
   public:
    ProcessTransactionOutputsJob(ProcessTransactionJob *parent):
      WriteBatchJob(parent, "ProcessTransactionOutputsJob"){}
    ~ProcessTransactionOutputsJob(){}

    TransactionPtr GetTransaction() const{
      return ((ProcessTransactionJob *) GetParent())->GetTransaction();
    }

    void Track(const UserHashLists &hashes){
      return ((ProcessTransactionJob *) GetParent())->Track(hashes);
    }

    void Append(leveldb::WriteBatch *batch){
      return ((ProcessTransactionJob *) GetParent())->Append(batch);
    }
  };
}

#endif //TOKEN_PROCESS_TRANSACTION_H
