#ifndef TOKEN_PROCESS_BLOCK_H
#define TOKEN_PROCESS_BLOCK_H

#include "block.h"
#include "job/job.h"

namespace Token{
    class ProcessBlockJob : public Job, BlockVisitor{
        friend class ProcessTransactionJob;
    protected:
        BlockPtr block_;
        bool clean_;

        JobResult DoWork(){
            if(!GetBlock()->Accept(this))
                return Failed("Cannot visit the block transactions.");
            return Success("Finished.");
        }
    public:
        ProcessBlockJob(const BlockPtr& blk, bool clean=false):
            Job(nullptr, "ProcessBlock"),
            block_(blk),
            clean_(clean){}
        ~ProcessBlockJob() = default;

        BlockPtr GetBlock() const{
            return block_;
        }

        bool ShouldClean() const{
            return clean_;
        }

        bool Visit(const TransactionPtr& tx);
    };
}

#endif //TOKEN_PROCESS_BLOCK_H