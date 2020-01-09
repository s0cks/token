#include <fstream>
#include "allocator.h"
#include "reporter.h"
#include "printer.h"

namespace Token{
    CrashReporter::~CrashReporter(){
        if(ShouldCloseOnWrite()){
            stream_.close();
        }
    }

    bool CrashReporter::WriteReport(){
        stream_ << "Token Node Crash Report" << std::endl;
        stream_ << "Version: " << "v1.0.0" << std::endl; //TODO implement version
        stream_ << "Message: " << GetMessage() << std::endl;
        if(ShouldWriteHeapInformation()){
            stream_ << std::endl;
            HeapPrinter::PrintHeap(GetStream(), Allocator::kEden, Printer::kDetailed);
            stream_ << std::endl;
            HeapPrinter::PrintHeap(GetStream(), Allocator::kSurvivor, Printer::kDetailed);
        }

        if(ShouldWriteBlockChainInformation()){
            stream_ << std::endl;
            BlockChainPrinter::Print(GetStream(), Printer::kDetailed);
        }

        if(ShouldWriteUnclaimedTransactionPoolInformation()){
            stream_ << std::endl;
            UnclaimedTransactionPoolPrinter::Print(GetStream());
        }
        return true;
    }
}