#include <string>
#include <sstream>
#include <glog/logging.h>
#include "printer.h"

namespace Token {

#define PRINT(Message) ({ \
    if(HasStream()){ \
        (*GetStream()) << (Message.str()) << std::endl; \
    } else{ \
        LOG_AT_LEVEL(GetSeverity()) << (Message.str()); \
    } \
})

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

        PRINT(stream);
        return true;
    }

    bool TransactionPrinter::VisitInputsStart(){
        if(IsDetailed()){
            std::stringstream stream;
            size_t num = GetTransaction()->GetNumberOfInputs();
            stream << "Inputs (" << num << "):";
            PRINT(stream);
        }
        return true;
    }

    bool TransactionPrinter::VisitInput(Token::Input *input){
        if(IsDetailed() && ShouldPrintBanner()) {
            std::stringstream stream;
            stream << "  - " << input->GetIndex() << ": " << input->GetHash();
            PRINT(stream);
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
            PRINT(stream);
        }
        return true;
    }

    bool TransactionPrinter::VisitOutput(Token::Output *output){
        if(IsDetailed()) {
            std::stringstream stream;
            stream << "  - " << output->GetUser() << " (" << output->GetToken() << ") -> " << output->GetHash();
            PRINT(stream);
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
            PRINT(msg);
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
        PRINT(stream);
    }

    bool BlockPrinter::VisitTransaction(Token::Transaction* tx){
        if(IsDetailed()){
            std::stringstream stream;
            stream << "  - " << tx->GetIndex() << ": " << tx->GetHash();
            PRINT(stream);
        }
    }

    bool BlockPrinter::VisitBlockEnd(){
        if(IsDetailed()){
            std::stringstream stream;
            stream << "Transactions: " << GetBlock()->GetNumberOfTransactions();

            stream = std::stringstream();
            for(size_t i = 0; i < BlockPrinter::kBannerSize; i++) stream << "*";
            PRINT(stream);
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
            total = (total - title.length() - 2) / 2;

            for (size_t i = 0; i < total; i++) stream << "*";
            stream << " " << title << " ";
            for (size_t i = 0; i < total; i++) stream << "*";
        } else{
            stream << "BlockChain (" << BlockChain::GetHeight() << " Blocks)";
        }
        PRINT(stream);
    }

    bool BlockChainPrinter::Visit(Token::Block* block){
        std::stringstream stream;
        stream << "  #" << block->GetHeight() << ": " << block->GetHash();
        if(IsDetailed()){
            stream << " (" << block->GetNumberOfTransactions() << " Transactions)";
        }
        PRINT(stream);
    }

    bool BlockChainPrinter::VisitEnd(){
        if(IsDetailed()){
            std::stringstream stream;
            stream = std::stringstream();
            for(size_t i = 0; i < BlockChainPrinter::kBannerSize; i++) stream << "*";
            PRINT(stream);
        }
    }

    void BlockChainPrinter::Print(std::ostream* stream, long flags){
        BlockChainPrinter printer(stream, flags);
        BlockChain::Accept(&printer);
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
        PRINT(stream);
    }

    bool TransactionPoolPrinter::VisitTransaction(Token::Transaction* tx){
        if(IsDetailed()){
            TransactionPrinter printer(GetSeverity(), tx, Printer::kDetailed|Printer::kNoBanner);
            tx->Accept(&printer);
        } else{
            std::stringstream stream;
            stream << "  - " << tx->GetHash();
            PRINT(stream);
        }
    }

    bool TransactionPoolPrinter::VisitEnd(){
        std::stringstream stream;
        stream = std::stringstream();
        for(size_t i = 0; i < TransactionPoolPrinter::kBannerSize; i++) stream << "*";
        PRINT(stream);
    }

    void TransactionPoolPrinter::Print(google::LogSeverity severity, long flags){
        TransactionPoolPrinter printer(severity, flags);
        TransactionPool::Accept(&printer);
    }

    bool UnclaimedTransactionPoolPrinter::VisitStart(){
        std::stringstream stream;
        size_t total = UnclaimedTransactionPoolPrinter::kBannerSize;

        std::string title = "UnclaimedTransactionPool";
        total = (total - title.length() - 2) / 2;

        for (size_t i = 0; i < total; i++) stream << "*";
        stream << " " << title << " ";
        for (size_t i = 0; i < total; i++) stream << "*";
        PRINT(stream);
        return true;
    }

    bool UnclaimedTransactionPoolPrinter::VisitUnclaimedTransaction(Token::UnclaimedTransaction* utx){
        std::stringstream stream;
        if(IsDetailed()){
            //TODO: Implement
        } else{
            stream << "  - " << utx->GetHash();
        }
        PRINT(stream);
        return true;
    }

    bool UnclaimedTransactionPoolPrinter::VisitEnd(){
        std::stringstream stream;
        stream = std::stringstream();
        for(size_t i = 0; i < UnclaimedTransactionPoolPrinter::kBannerSize; i++) stream << "*";
        PRINT(stream);
        return true;
    }

    void UnclaimedTransactionPoolPrinter::Print(std::ostream *stream, long flags){
        UnclaimedTransactionPoolPrinter printer(stream, flags);
        UnclaimedTransactionPool::GetInstance()->Accept(&printer);
    }

    void UnclaimedTransactionPoolPrinter::Print(google::LogSeverity severity, long flags){
        UnclaimedTransactionPoolPrinter printer(severity, flags);
        UnclaimedTransactionPool::GetInstance()->Accept(&printer);
    }
}