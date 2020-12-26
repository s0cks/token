#ifndef TOKEN_BLOCK_PROCESSOR_H
#define TOKEN_BLOCK_PROCESSOR_H

#include "block.h"
#include "transaction_processor.h"

#include "job/job.h"
#include "job/scheduler.h"
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