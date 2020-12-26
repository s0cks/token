#ifndef TOKEN_PROCESS_BLOCK_H
#define TOKEN_PROCESS_BLOCK_H

#include "block.h"
#include "job/job.h"

namespace Token{
    class ProcessBlockJob : public Job, BlockVisitor{
        friend class ProcessTransactionJob;
    protected:
        BlockPtr block_;
        std::mutex mutex_;

        JobResult DoWork(){
            if(!GetBlock()->Accept(this))
                return Failed("Cannot visit the block transactions.");
            return Success("Finished.");
        }
    public:
        ProcessBlockJob(const BlockPtr& blk):
            Job(nullptr, "ProcessBlock"),
            block_(blk){}
        ~ProcessBlockJob() = default;

        BlockPtr GetBlock() const{
            return block_;
        }

        bool Visit(const TransactionPtr& tx);
    };
}

#endif //TOKEN_PROCESS_BLOCK_H