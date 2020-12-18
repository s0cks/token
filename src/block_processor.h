#ifndef TOKEN_BLOCK_PROCESSOR_H
#define TOKEN_BLOCK_PROCESSOR_H

#include "block.h"
#include "transaction_processor.h"

#include "task/job.h"
#include "utils/timeline.h"

namespace Token{
    class BlockProcessor : public BlockVisitor{
    protected:
        Timeline timeline_;

        BlockProcessor(const std::string& timeline_name="ProcessBlock"):
            BlockVisitor(),
            timeline_(timeline_name){}

        virtual bool Process(const TransactionPtr& tx) = 0;

        static inline std::string
        GetProcessTransactionTimeSliceName(int64_t index){
            std::stringstream ss;
            ss << "Process#" << index;
            return ss.str();
        }
    public:
        virtual ~BlockProcessor() = default;

        Timeline& GetTimeline(){
            return timeline_;
        }

        Timeline GetTimeline() const{
            return timeline_;
        }

        bool VisitStart(){
            timeline_ << "Start";
            return true;
        }

        bool Visit(const TransactionPtr& tx){
            timeline_ << GetProcessTransactionTimeSliceName(tx->GetIndex());
            return Process(tx);
        }

        bool VisitEnd(){
            timeline_ << "Stop";
            return true;
        }
    };

    class ProcessBlockJob;
    class ProcessTransactionJob : public Job{
    protected:
        TransactionPtr transaction_;

        JobResult DoWork();
    public:
        ProcessTransactionJob(ProcessBlockJob* parent, const TransactionPtr& tx);
        ~ProcessTransactionJob() = default;

        TransactionPtr GetTransaction() const{
            return transaction_;
        }
    };

    class ProcessBlockJob : public Job, BlockVisitor{
        friend class ProcessTransactionJob;
    protected:
        BlockPtr block_;

        std::mutex mutex_;
        std::set<Hash> valid_;
        std::set<Hash> invalid_;

        JobResult DoWork(){
            if(!GetBlock()->Accept(this))
                return Failed("Cannot visit the block transactions.");
            return Success("Finished.");
        }

        bool AddValid(const Hash& hash){
            std::unique_lock<std::mutex> lock(mutex_);
            return valid_.insert(hash).second;
        }

        bool AddInvalid(const Hash& hash){
            std::unique_lock<std::mutex> lock(mutex_);
            return invalid_.insert(hash).second;
        }
    public:
        ProcessBlockJob(Job* parent, const BlockPtr& blk):
            Job(parent, "ProcessBlock"),
            block_(blk){}
        ProcessBlockJob(const BlockPtr& blk):
            Job(nullptr, "ProcessBlock"),
            block_(blk){}
        ~ProcessBlockJob() = default;

        BlockPtr GetBlock() const{
            return block_;
        }

        bool Visit(const TransactionPtr& tx){
            //TODO: fix memory leak
            ProcessTransactionJob* job = new ProcessTransactionJob(this, tx);
            return JobPool::Schedule(job);
        }

        std::set<Hash> GetValid(){
            std::unique_lock<std::mutex> lock(mutex_);
            return valid_;
        }

        std::set<Hash> GetInvalid(){
            std::unique_lock<std::mutex> lock(mutex_);
            return invalid_;
        }
    };

    class GenesisBlockProcessor : public BlockProcessor{
    protected:
        GenesisBlockProcessor(): BlockProcessor("ProcessGenesisBlock"){}

        bool Process(const TransactionPtr& tx){
            Hash hash = tx->GetHash();
            if(!TransactionHandler::ProcessTransaction(tx)){
                LOG(WARNING) << "couldn't process transaction: " << hash;
                return false;
            }

            return true;
        }
    public:
        ~GenesisBlockProcessor() = default;

        static bool
        Process(const BlockPtr& blk){
            GenesisBlockProcessor processor;
            bool success = true;

            if(!(success = blk->Accept(&processor))){
                LOG(WARNING) << "couldn't visit block transactions.";
                goto stop;
            }
        stop:
#ifdef TOKEN_DEBUG
            TimelinePrinter printer(google::INFO, Printer::kFlagDetailed);
            printer.Print(processor.GetTimeline());
#endif//TOKEN_DEBUG
            return success;
        }
    };

    class SynchronizeBlockProcessor : public BlockProcessor{
        //TODO: rename SynchronizeBlockProcessor
    protected:
        SynchronizeBlockProcessor(): BlockProcessor("ProcessSynchronizedBlock"){}

        bool Process(const TransactionPtr& tx){
            Hash hash = tx->GetHash();
            if(!TransactionHandler::ProcessTransaction(tx)){
                LOG(WARNING) << "couldn't process transaction: " << hash;
                return false;
            }

            return true;
        }
    public:
        ~SynchronizeBlockProcessor() = default;

        static bool
        Process(const BlockPtr& blk){
            SynchronizeBlockProcessor processor;
            bool success = true;

            if(!(success = blk->Accept(&processor))){
                LOG(WARNING) << "couldn't visit block transactions.";
                goto stop;
            }
            stop:
#ifdef TOKEN_DEBUG
            TimelinePrinter printer(google::INFO, Printer::kFlagDetailed);
            printer.Print(processor.GetTimeline());
#endif//TOKEN_DEBUG
            return success;
        }
    };

    class DefaultBlockProcessor : public BlockProcessor{
    protected:
        DefaultBlockProcessor(): BlockProcessor("ProcessDefaultBlock"){}

        bool Process(const TransactionPtr& tx){
            Hash hash = tx->GetHash();
            if(!TransactionHandler::ProcessTransaction(tx)){
                LOG(WARNING) << "couldn't process transaction: " << hash;
                return false;
            }

            if(!TransactionPool::RemoveTransaction(hash)){
                LOG(WARNING) << "couldn't remove transaction " << hash << " from pool.";
                return false;
            }

            return true;
        }
    public:
        ~DefaultBlockProcessor() = default;

        static bool
        Process(const BlockPtr& blk){
            DefaultBlockProcessor processor;
            bool success = true;

            if(!(success = blk->Accept(&processor))){
                LOG(WARNING) << "couldn't visit block transactions.";
                goto stop;
            }
        stop:
#ifdef TOKEN_DEBUG
            TimelinePrinter printer(google::INFO, Printer::kFlagDetailed);
            printer.Print(processor.GetTimeline());
#endif//TOKEN_DEBUG
            return success;
        }
    };
}

#endif //TOKEN_BLOCK_PROCESSOR_H