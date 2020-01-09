#include <string>
#include <sstream>
#include <glog/logging.h>
#include "printer.h"

namespace Token {
#define PRINTER (LOG_AT_LEVEL(GetSeverity()))

    bool HeapPrinter::VisitStart() {
        std::stringstream msg;

        int total = HeapPrinter::BANNER_SIZE;
        std::string title = space_ == kEden ?
                            " Eden " :
                            " Survivor ";

        total = (total - title.length()) / 2;
        for (int i = 0; i < total; i++) {
            msg << "*";
        }
        msg << title;
        for (int i = 0; i < total; i++) {
            msg << "*";
        }
        LOG(INFO) << msg.str();
        return true;
    }

    bool HeapPrinter::VisitEnd() {
        std::stringstream msg;
        for (int i = 0; i < HeapPrinter::BANNER_SIZE; i++) {
            msg << "*";
        }
        LOG(INFO) << msg.str();
        return true;
    }

#define WITH_HEADER(size) ((size)+sizeof(int))
#define BITS(ch) (WITH_HEADER(ch))
#define FLAGS(ch) (*((uint32_t*)(ch)))
#define MARKED(ch) (FLAGS(ch)&1)

    bool HeapPrinter::VisitChunk(int chunk, size_t size, void *ptr) {
        std::stringstream msg;
        try {
            AllocatorObject *obj = reinterpret_cast<AllocatorObject *>(BITS(ptr));
            if (obj) {
                msg << obj->ToString();
            } else {
                msg << "\tChunk " << chunk << "\tSize " << size << "\tMarked: " << (MARKED(ptr) ? 'Y' : 'N');
            }
        } catch (std::exception &exc) {
            msg << "\tChunk " << chunk << "\tSize " << size << "\tMarked: " << (MARKED(ptr) ? 'Y' : 'N');
        }
        LOG(INFO) << msg.str();
        return true;
    }

    bool TransactionPrinter::VisitStart() {
        std::stringstream stream;
        if (IsDetailed()) {
            std::string title = "Transaction " + GetTransaction()->GetHash();
            if(ShouldPrintBanner()){
                size_t total = (TransactionPrinter::kBannerSize - title.length()) / 2;
                for (size_t i = 0; i < total; i++) stream << "*";
                stream << " " << title << " ";
                for (size_t i = 0; i < total; i++) stream << "*";
            } else{
                stream << title;
            }
        } else {
            stream << GetTransaction()->GetHash();
        }
        PRINTER << stream.str();
        return true;
    }

    bool TransactionPrinter::VisitInputsStart(){
        if(IsDetailed()){
            std::stringstream stream;
            size_t num = GetTransaction()->GetNumberOfInputs();
            stream << "Inputs (" << num << "):";
            PRINTER << stream.str();
        }
        return true;
    }

    bool TransactionPrinter::VisitInput(Token::Input *input){
        if(IsDetailed() && ShouldPrintBanner()) {
            std::stringstream stream;
            stream << "  - " << input->GetIndex() << ": " << input->GetHash();
            PRINTER << stream.str();
        }
        return true;
    }

    bool TransactionPrinter::VisitInputsEnd(){
        return true;
    }

    bool TransactionPrinter::VisitOutputsStart(){
        if(IsDetailed()){
            std::stringstream stream;
            size_t num = GetTransaction()->GetNumberOfOutputs();
            stream << "Outputs (" << num << "):";
            PRINTER << stream.str();
        }
        return true;
    }

    bool TransactionPrinter::VisitOutput(Token::Output *output){
        if(IsDetailed()) {
            std::stringstream stream;
            stream << "  - " << output->GetUser() << " (" << output->GetToken() << ") -> " << output->GetHash();
            PRINTER << stream.str();
        }
        return true;
    }

    bool TransactionPrinter::VisitOutputsEnd(){
        return true;
    }

    bool TransactionPrinter::VisitEnd(){
        if(IsDetailed()){
            std::stringstream msg;
            for(size_t i = 0; i < TransactionPrinter::kBannerSize; i++){
                msg << "*";
            }
            PRINTER << msg.str();
        }
        return true;
    }

    void TransactionPrinter::Print(google::LogSeverity sv, Token::Transaction *tx, bool detailed){
        TransactionPrinter printer(sv, tx, detailed);
        tx->Accept(&printer);
    }

    bool BlockPrinter::VisitBlockStart(){
        std::stringstream stream;
        if(IsDetailed()){
            size_t total = BlockPrinter::kBannerSize;

            std::stringstream height;
            height << GetBlock()->GetHeight();
            std::string title = "Block #" + height.str();

            total = (total - title.length()) / 2;

            for (size_t i = 0; i < total; i++) stream << "*";
            stream << " " << title << " ";
            for (size_t i = 0; i < total; i++) stream << "*";
        } else{
            stream << GetBlock()->GetHash();
        }
        PRINTER << stream.str();
    }

    bool BlockPrinter::VisitTransaction(Token::Transaction* tx){
        if(IsDetailed()){
            std::stringstream stream;
            stream << "  - " << tx->GetIndex() << ": " << tx->GetHash();
            PRINTER << stream.str();
        }
    }

    bool BlockPrinter::VisitBlockEnd(){
        if(IsDetailed()){
            std::stringstream stream;
            stream << "Transactions: " << GetBlock()->GetNumberOfTransactions();

            stream = std::stringstream();
            for(size_t i = 0; i < BlockPrinter::kBannerSize; i++) stream << "*";
            PRINTER << stream.str();
        }
    }

    void BlockPrinter::Print(google::LogSeverity severity, Token::Block *block, long flags){
        BlockPrinter printer(severity, block, flags);
        block->Accept(&printer);
    }

    bool BlockChainPrinter::VisitStart(){
        std::stringstream stream;
        if(IsDetailed()){
            size_t total = BlockChainPrinter::kBannerSize;

            std::string title = "BlockChain";
            total = (total - title.length()) / 2;

            for (size_t i = 0; i < total; i++) stream << "*";
            stream << " " << title << " ";
            for (size_t i = 0; i < total; i++) stream << "*";
        } else{
            stream << "BlockChain (" << BlockChain::GetHeight() << " Blocks)";
        }
        PRINTER << stream.str();
    }

    bool BlockChainPrinter::Visit(Token::Block* block){
        std::stringstream stream;
        stream << "  #" << block->GetHeight() << ": " << block->GetHash();
        if(IsDetailed()){
            stream << " (" << block->GetNumberOfTransactions() << " Transactions)";
        }
        PRINTER << stream.str();
    }

    bool BlockChainPrinter::VisitEnd(){
        if(IsDetailed()){
            std::stringstream stream;
            stream = std::stringstream();
            for(size_t i = 0; i < BlockChainPrinter::kBannerSize; i++) stream << "*";
            PRINTER << stream.str();
        }
    }

    void BlockChainPrinter::Print(google::LogSeverity severity, long flags){
        BlockChainPrinter printer(severity, flags);
        BlockChain::Accept(&printer);
    }

    bool TransactionPoolPrinter::VisitStart(){
        std::stringstream stream;
        size_t total = TransactionPoolPrinter::kBannerSize;

        std::string title = "TransactionPool";
        total = (total - title.length()) / 2;

        for (size_t i = 0; i < total; i++) stream << "*";
        stream << " " << title << " ";
        for (size_t i = 0; i < total; i++) stream << "*";
        PRINTER << stream.str();
    }

    bool TransactionPoolPrinter::VisitTransaction(Token::Transaction* tx){
        if(IsDetailed()){
            TransactionPrinter printer(GetSeverity(), tx, Printer::kDetailed|Printer::kNoBanner);
            tx->Accept(&printer);
        } else{
            PRINTER << "  - " << tx->GetHash();
        }
    }

    bool TransactionPoolPrinter::VisitEnd(){
        std::stringstream stream;
        stream = std::stringstream();
        for(size_t i = 0; i < TransactionPoolPrinter::kBannerSize; i++) stream << "*";
        PRINTER << stream.str();
    }

    void TransactionPoolPrinter::Print(google::LogSeverity severity, long flags){
        TransactionPoolPrinter printer(severity, flags);
        TransactionPool::Accept(&printer);
    }

    bool UnclaimedTransactionPoolPrinter::VisitStart(){
        std::stringstream stream;
        size_t total = UnclaimedTransactionPoolPrinter::kBannerSize;

        std::string title = "UnclaimedTransactionPool";
        total = (total - title.length()) / 2;

        for (size_t i = 0; i < total; i++) stream << "*";
        stream << " " << title << " ";
        for (size_t i = 0; i < total; i++) stream << "*";
        PRINTER << stream.str();
    }

    bool UnclaimedTransactionPoolPrinter::VisitUnclaimedTransaction(Token::UnclaimedTransaction* utx){
        std::stringstream stream;
        if(IsDetailed()){
            //TODO: Implement
        } else{
            stream << "  - " << utx->GetHash();
        }
        PRINTER << stream.str();
    }

    bool UnclaimedTransactionPoolPrinter::VisitEnd(){
        std::stringstream stream;
        stream = std::stringstream();
        for(size_t i = 0; i < UnclaimedTransactionPoolPrinter::kBannerSize; i++) stream << "*";
        PRINTER << stream.str();
    }

    void UnclaimedTransactionPoolPrinter::Print(google::LogSeverity severity, long flags){
        UnclaimedTransactionPoolPrinter printer(severity, flags);
        UnclaimedTransactionPool::GetInstance()->Accept(&printer);
    }
}