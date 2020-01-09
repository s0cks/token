#ifndef TOKEN_PRINTER_H
#define TOKEN_PRINTER_H

#include "allocator.h"
#include "blockchain.h"

namespace Token{
    class HeapPrinter : public HeapVisitor{
    public:
        static const int BANNER_SIZE = 64;

        enum HeapSpace{
            kEden,
            kSurvivor
        };
    private:
        HeapSpace space_;
    public:
        HeapPrinter(HeapSpace space):
                space_(space){}
        ~HeapPrinter(){}

        bool VisitStart();
        bool VisitEnd();
        bool VisitChunk(int chunk, size_t size, void* ptr);
    };

    class TransactionPrinter : public TransactionVisitor{
    public:
        static const size_t kBannerSize = 82;
    private:
        google::LogSeverity severity_;
        Transaction* tx_;
        bool detailed_;
    public:
        TransactionPrinter(google::LogSeverity severity, Transaction* tx, bool is_detailed):
            tx_(tx),
            severity_(severity),
            detailed_(is_detailed){}
        ~TransactionPrinter(){}

        bool IsDetailed(){
            return detailed_;
        }

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

    class BlockPrinter : public BlockVisitor{
    public:
        static const size_t kBannerSize = 64;
    private:
        bool detailed_;
        google::LogSeverity severity_;
        Block* block_;
    public:
        BlockPrinter(google::LogSeverity severity, Block* block, bool detailed):
            severity_(severity),
            block_(block),
            detailed_(detailed){}
        ~BlockPrinter(){}

        bool IsDetailed(){
            return detailed_;
        }

        Block* GetBlock(){
            return block_;
        }

        bool VisitBlockStart();
        bool VisitTransaction(Transaction* tx);
        bool VisitBlockEnd();

        static void Print(google::LogSeverity severity, Block* block, bool detailed=false);

        static void PrintAsInfo(Block* block, bool detailed=false){
            Print(google::INFO, block, detailed);
        }

        static void PrintAsError(Block* block, bool detailed=false){
            Print(google::ERROR, block, detailed);
        }
    };
}

#endif //TOKEN_PRINTER_H