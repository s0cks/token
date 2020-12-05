#ifndef TOKEN_BLOCK_CHAIN_PRINTER_H
#define TOKEN_BLOCK_CHAIN_PRINTER_H

#include "printer.h"
#include "block_chain.h"

namespace Token{
    class BlockChainPrinter : public Printer, public BlockChainVisitor{
    public:
        static const int kFlagsNone = 0;
        static const int kFlagsDetailed = 1 << 1;
    private:
        int flags_;
    public:
        BlockChainPrinter(int flags, const google::LogSeverity& severity=google::INFO):
            Printer(severity),
            flags_(flags){}
        ~BlockChainPrinter() = default;

        bool Visit(Block* blk){

            return true;
        }
    };
}

#endif //TOKEN_BLOCK_CHAIN_PRINTER_H