#include <string>
#include <sstream>
#include <glog/logging.h>
#include "printer.h"

namespace Token {
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
            size_t total = TransactionPrinter::kBannerSize;
            std::string title = "Transaction " + GetTransaction()->GetHash();

            total = (total - title.length()) / 2;

            for (size_t i = 0; i < total; i++) stream << "*";
            stream << " " << title << " ";
            for (size_t i = 0; i < total; i++) stream << "*";
        } else {
            stream << GetTransaction()->GetHash();
        }
        LOG_AT_LEVEL(severity_) << stream.str();
        return true;
    }

    bool TransactionPrinter::VisitInputsStart(){
        if(IsDetailed()){
            std::stringstream stream;
            size_t num = GetTransaction()->GetNumberOfInputs();
            stream << "Inputs (" << num << "):";
            LOG_AT_LEVEL(severity_) << stream.str();
        }
        return true;
    }

    bool TransactionPrinter::VisitInput(Token::Input *input){
        if(IsDetailed()) {
            std::stringstream stream;
            stream << "  - " << input->GetIndex() << ": " << input->GetHash();
            LOG_AT_LEVEL(severity_) << stream.str();
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
            LOG_AT_LEVEL(severity_) << stream.str();
        }
        return true;
    }

    bool TransactionPrinter::VisitOutput(Token::Output *output){
        if(IsDetailed()) {
            std::stringstream stream;
            stream << "  - " << output->GetUser() << " (" << output->GetToken() << ") -> " << output->GetHash();
            LOG_AT_LEVEL(severity_) << stream.str();
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
            LOG_AT_LEVEL(severity_) << msg.str();
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
        LOG_AT_LEVEL(severity_) << stream.str();
    }

    bool BlockPrinter::VisitTransaction(Token::Transaction* tx){
        if(IsDetailed()){
            std::stringstream stream;
            stream << "  - " << tx->GetIndex() << ": " << tx->GetHash();
            LOG_AT_LEVEL(severity_) << stream.str();
        }
    }

    bool BlockPrinter::VisitBlockEnd(){
        if(IsDetailed()){
            std::stringstream stream;
            stream << "Transactions: " << GetBlock()->GetNumberOfTransactions();

            stream = std::stringstream();
            for(size_t i = 0; i < BlockPrinter::kBannerSize; i++) stream << "*";
            LOG_AT_LEVEL(severity_) << stream.str();
        }
    }

    void BlockPrinter::Print(google::LogSeverity severity, Token::Block *block, bool detailed){
        BlockPrinter printer(severity, block, detailed);
        block->Accept(&printer);
    }
}