#ifndef TOKEN_PRINTER_H
#define TOKEN_PRINTER_H

#include <glog/logging.h>
#include "blockchain.h"

namespace Token{
    class Printer{
    public:
        enum PrinterFlags{
            kNone = 0,
            kDetailed = 1 << 1,
            kNoBanner = 1 << 2
        };
    private:
        long flags_;
        bool is_log_;
        std::ostream* stream_;
        google::LogSeverity severity_;
    protected:
        Printer(std::ostream* stream, long flags):
            flags_(flags),
            is_log_(false),
            severity_(0),
            stream_(stream){}
        Printer(google::LogSeverity severity, long flags):
            flags_(flags),
            severity_(severity),
            stream_(nullptr),
            is_log_(true){}
    public:
        virtual ~Printer(){}

        google::LogSeverity GetSeverity(){
            return severity_;
        }

        std::ostream* GetStream(){
            return stream_;
        }

        bool HasStream(){
            return stream_ != nullptr;
        }

        bool IsDetailed(){
            return (flags_ & kDetailed) == kDetailed;
        }

        bool ShouldPrintBanner(){
            return !((flags_ & kNoBanner) == kNoBanner);
        }
    };

    class TransactionPrinter : public TransactionVisitor, public Printer{
    public:
        static const size_t kBannerSize = 96;
    private:
        Transaction* tx_;
    public:
        TransactionPrinter(google::LogSeverity severity, Transaction* tx, long flags):
            tx_(tx),
            TransactionVisitor(),
            Printer(severity, flags){}
        ~TransactionPrinter(){}

        Transaction* GetTransaction(){
            return tx_;
        }

        bool VisitStart();

        bool VisitInputsStart();
        bool VisitInput(Input* input);
        bool VisitInputsEnd();

        bool VisitOutputsStart();
        bool VisitOutput(Output* output);
        bool VisitOutputsEnd();

        bool VisitEnd();

        static void Print(google::LogSeverity sv, Transaction* tx, bool detailed=false);

        static void PrintAsInfo(Transaction* tx, bool detailed=false){
            Print(google::INFO, tx, detailed);
        }

        static void PrintAsError(Transaction* tx, bool detailed=false){
            Print(google::ERROR, tx, detailed);
        }
    };

    class BlockPrinter : public BlockVisitor, public Printer{
    public:
        static const size_t kBannerSize = 96;
    private:
        Block* block_;
    public:
        BlockPrinter(google::LogSeverity severity, Block* block, long flags):
            block_(block),
            BlockVisitor(),
            Printer(severity, flags){}
        ~BlockPrinter(){}

        Block* GetBlock(){
            return block_;
        }

        bool VisitBlockStart();
        bool VisitTransaction(Transaction* tx);
        bool VisitBlockEnd();

        static void Print(google::LogSeverity severity, Block* block, long flags=Printer::kNone);

        static void PrintAsInfo(Block* block, long flags=Printer::kNone){
            Print(google::INFO, block, flags);
        }

        static void PrintAsError(Block* block, long flags=Printer::kNone){
            Print(google::ERROR, block, flags);
        }
    };

    class BlockChainPrinter : public BlockChainVisitor, public Printer{
    public:
        static const size_t kBannerSize = 96;

        BlockChainPrinter(std::ostream* stream, long flags):
            BlockChainVisitor(),
            Printer(stream, flags){}
        BlockChainPrinter(google::LogSeverity severity, long flags):
            BlockChainVisitor(),
            Printer(severity, flags){}
        ~BlockChainPrinter(){}

        bool VisitStart();
        bool Visit(Block* block);
        bool VisitEnd();

        static void Print(std::ostream* stream, long flags=Printer::kNone);
        static void Print(google::LogSeverity severity, long flags=Printer::kNone);

        static void PrintAsInfo(long flags=Printer::kNone){
            Print(google::INFO, flags);
        }

        static void PrintAsError(long flags=Printer::kNone){
            Print(google::ERROR, flags);
        }
    };

    class TransactionPoolPrinter : public TransactionPoolVisitor, public Printer{
    public:
        static const size_t kBannerSize = 96;

        TransactionPoolPrinter(google::LogSeverity severity, long flags):
            TransactionPoolVisitor(),
            Printer(severity, flags){}
        ~TransactionPoolPrinter(){}

        bool VisitStart();
        bool VisitTransaction(Transaction* tx);
        bool VisitEnd();

        static void Print(google::LogSeverity severity, long flags=Printer::kNone);

        static void PrintAsInfo(long flags=Printer::kNone){
            Print(google::INFO, flags);
        }

        static void PrintAsError(long flags=Printer::kNone){
            Print(google::ERROR, flags);
        }
    };

    class UnclaimedTransactionPoolPrinter : public UnclaimedTransactionPoolVisitor, public Printer{
    public:
        static const size_t kBannerSize = 96;

        UnclaimedTransactionPoolPrinter(std::ostream* stream, long flags):
            UnclaimedTransactionPoolVisitor(),
            Printer(stream, flags){}
        UnclaimedTransactionPoolPrinter(google::LogSeverity severity, long flags):
            UnclaimedTransactionPoolVisitor(),
            Printer(severity, flags){}
        ~UnclaimedTransactionPoolPrinter(){}

        bool VisitStart();
        bool VisitUnclaimedTransaction(UnclaimedTransaction* utx);
        bool VisitEnd();

        static void Print(std::ostream* stream, long flags=Printer::kNone);
        static void Print(google::LogSeverity severity, long flags=Printer::kNone);

        static void PrintAsInfo(long flags=Printer::kNone){
            Print(google::INFO, flags);
        }

        static void PrintAsError(long flags=Printer::kNone){
            Print(google::ERROR, flags);
        }
    };
}

#endif //TOKEN_PRINTER_H