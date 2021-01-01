#ifndef TOKEN_PROCESS_TRANSACTION_H
#define TOKEN_PROCESS_TRANSACTION_H

#include <leveldb/write_batch.h>
#include "job/job.h"
#include "job/process_block.h"
#include "utils/atomic_linked_list.h"

namespace Token{
  class ProcessTransactionJob : public Job{
   protected:
    TransactionPtr transaction_;

    JobResult DoWork();
   public:
    ProcessTransactionJob(ProcessBlockJob* parent, const TransactionPtr& tx):
      Job(parent, "ProcessTransaction"),
      transaction_(tx){}
    ~ProcessTransactionJob() = default;

    BlockPtr GetBlock() const{
      return ((ProcessBlockJob*) GetParent())->GetBlock();
    }

    bool ShouldClean() const{
      return ((ProcessBlockJob*) GetParent())->ShouldClean();
    }

    TransactionPtr GetTransaction() const{
      return transaction_;
    }

    void Append(leveldb::WriteBatch* batch, const UserHashLists& hashes){
      return ((ProcessBlockJob*) GetParent())->Append(batch, hashes);
    }
  };

  class ProcessTransactionInputsJob : public Job{
   protected:
    JobResult DoWork();
   public:
    ProcessTransactionInputsJob(ProcessTransactionJob* parent):
      Job(parent, "ProcessTransactionInputsJob"){}
    ~ProcessTransactionInputsJob() = default;

    TransactionPtr GetTransaction() const{
      return ((ProcessTransactionJob*) GetParent())->GetTransaction();
    }

    void Append(leveldb::WriteBatch* batch){
      return ((ProcessTransactionJob*) GetParent())->Append(batch, UserHashLists());
    }
  };

  class ProcessTransactionOutputsJob : public Job{
   protected:
    JobResult DoWork();
   public:
    ProcessTransactionOutputsJob(ProcessTransactionJob* parent):
      Job(parent, "ProcessTransactionOutputsJob"){}
    ~ProcessTransactionOutputsJob(){}

    TransactionPtr GetTransaction() const{
      return ((ProcessTransactionJob*) GetParent())->GetTransaction();
    }

    void Append(leveldb::WriteBatch* batch, const UserHashLists& user_hashes){
      return ((ProcessTransactionJob*) GetParent())->Append(batch, user_hashes);
    }
  };
}

#endif //TOKEN_PROCESS_TRANSACTION_H
