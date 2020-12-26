#include "job/scheduler.h"
#include "job/process_block.h"
#include "job/process_transaction.h"

namespace Token{
    bool ProcessBlockJob::Visit(const TransactionPtr& tx){
        ProcessTransactionJob* job = new ProcessTransactionJob(this, tx);
        return JobScheduler::Schedule(job);
    }
}