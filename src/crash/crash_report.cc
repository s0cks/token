#include "crash/crash_report.h"

#include "blockchain.h"

namespace token{
  bool CrashReport::Print() const{
    Print("Cause: %s\n", cause_.data());
    Print("Timestamp: %s\n", FormatTimestampReadable(timestamp_).data());
    Print("Stack Trace:\n");

    //TODO: implement
    BlockChainPtr chain = BlockChain::GetInstance();
    if(!chain->IsInitialized()){
      Print("Block Chain: N/A\n");
    } else{
      Print("Block Chain: \n");
    }
    return true;
  }
}