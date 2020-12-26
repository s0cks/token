#ifndef TOKEN_PROCESS_TRANSACTION_H
#define TOKEN_PROCESS_TRANSACTION_H

#include "job/job.h"
#include "job/process_block.h"

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

        TransactionPtr GetTransaction() const{
            return transaction_;
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
            return ((ProcessTransactionJob*)GetParent())->GetTransaction();
        }
    };

    class ProcessTransactionOutputsJob : public Job{
    protected:
        JobResult DoWork();
    public:
        ProcessTransactionOutputsJob(ProcessTransactionJob* parent):
            Job(parent, "ProcessTransactionOutputsJob"){}
        ~ProcessTransactionOutputsJob() = default;

        TransactionPtr GetTransaction() const{
            return ((ProcessTransactionJob*)GetParent())->GetTransaction();
        }
    };
}

#endif //TOKEN_PROCESS_TRANSACTION_H
