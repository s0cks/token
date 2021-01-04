#include <glog/logging.h>
#include "pool.h"
#include "version.h"
#include "blockchain.h"
#include "utils/printer.h"
#include "utils/crash_report.h"

#ifdef TOKEN_ENABLE_SERVER
  #include "server/server.h"
  #include "peer/peer_session_manager.h"
#endif//TOKEN_ENABLE_SERVER

namespace Token{
  bool CrashReport::PrintNewCrashReport(const std::string& cause, const google::LogSeverity& severity){
    CrashReportPrinter printer(cause, severity);
    return printer.Print();
  }

  bool CrashReport::PrintNewCrashReportAndExit(const std::string& cause, const google::LogSeverity& severity, int code){
    CrashReportPrinter printer(cause, severity);
    return printer.Print();
  }

  bool CrashReportPrinter::PrintBanner(){
    std::string header = "Token v" + Token::GetVersion();
    size_t total_size = 50;
    size_t middle = (total_size - header.size()) / 2;

    std::stringstream ss1;
    for(size_t idx = 0; idx < total_size; idx++) ss1 << "#";

    std::stringstream ss2;
    ss2 << "#";
    for(size_t idx = 0; idx < middle; idx++) ss2 << " ";
    ss2 << header;
    for(size_t idx = 0; idx < middle - 2; idx++) ss2 << " ";
    ss2 << "#";

    LOG_AT_LEVEL(GetSeverity()) << ss1.str();
    LOG_AT_LEVEL(GetSeverity()) << ss2.str();
    LOG_AT_LEVEL(GetSeverity()) << ss1.str();
    return true;
  }

  bool CrashReportPrinter::PrintStackTrace(){
    LOG_AT_LEVEL(GetSeverity()) << "Stack Trace:";
    int32_t offset = 4;
    int32_t depth = 20;
    StackTrace trace(offset, depth);
    for(int32_t idx = offset; idx < trace.GetNumberOfFrames(); idx++)
      LOG_AT_LEVEL(GetSeverity()) << "\t" << (idx + 1 - offset) << ": " << trace[idx];
    return true;
  }

  bool CrashReportPrinter::PrintSystemInformation(){
    LOG_AT_LEVEL(GetSeverity()) << "Timestamp: " << GetTimestampFormattedReadable(GetCurrentTimestamp());
    LOG_AT_LEVEL(GetSeverity()) << "Version: " << Version();
    LOG_AT_LEVEL(GetSeverity()) << "Debug Mode: " << (TOKEN_DEBUG ? "Enabled" : "Disabled");
    LOG_AT_LEVEL(GetSeverity()) << "Ledger Home: " << TOKEN_BLOCKCHAIN_HOME;
    LOG_AT_LEVEL(GetSeverity()) << "Cause: " << GetCause();
#ifdef OS_IS_LINUX
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: Linux";
#elif OS_IS_WINDOWS
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: Windows";
#elif OS_IS_OSX
      LOG_AT_LEVEL(GetSeverity()) << "Operating System: OSX";
#endif //Operating System
#ifdef ARCHITECTURE_IS_X64
    LOG_AT_LEVEL(GetSeverity()) << "Architecture: x64";
#elif ARCHITECTURE_IS_X32
    LOG_AT_LEVEL(GetSeverity()) << "Architecture: x32";
#endif //Architecture

    if(BlockChain::IsInitialized() && BlockChain::IsOk()){
      LOG_AT_LEVEL(GetSeverity()) << "Block Chain (" << BlockChain::GetStatus() << "):";
      LOG_AT_LEVEL(GetSeverity()) << "\tGenesis: " << BlockChain::GetReference(BLOCKCHAIN_REFERENCE_GENESIS);
      LOG_AT_LEVEL(GetSeverity()) << "\tHead: " << BlockChain::GetReference(BLOCKCHAIN_REFERENCE_HEAD);
    } else{
      LOG_AT_LEVEL(GetSeverity()) << "Block Chain (" << BlockChain::GetStatus() << "): " << BlockChain::GetState();
    }

    if(ObjectPool::IsInitialized() && ObjectPool::IsOk()){
      LOG_AT_LEVEL(GetSeverity()) << "Object Pool (" << ObjectPool::GetStatus() << "):";
      LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Objects: " << ObjectPool::GetNumberOfObjects();
      LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Blocks: " << ObjectPool::GetNumberOfBlocks();
      LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Transactions: " << ObjectPool::GetNumberOfTransactions();
      LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Unclaimed Transactions: "
                                  << ObjectPool::GetNumberOfUnclaimedTransactions();
    } else{
      LOG_AT_LEVEL(GetSeverity()) << "Block Pool (" << ObjectPool::GetStatus() << "): " << ObjectPool::GetState();
    }

    #ifdef TOKEN_ENABLE_SERVER
    if(Server::IsRunning() && Server::IsOk()){
      LOG_AT_LEVEL(GetSeverity()) << "Server (" << Server::GetStatus() << "): ";

      std::set<UUID> peers;
      if(PeerSessionManager::GetConnectedPeers(peers)){
        LOG_AT_LEVEL(GetSeverity()) << "\tPeers:";
        for(auto& it : peers){
          std::shared_ptr<PeerSession> session = PeerSessionManager::GetSession(it);
          UUID id = session->GetID();
          NodeAddress addr = session->GetAddress();
          LOG_AT_LEVEL(GetSeverity()) << "\t - " << id << " (" << addr << "): " << session->GetState() << " ["
                                      << session->GetStatus() << "]";
        }
      }
    } else{
      LOG_AT_LEVEL(GetSeverity()) << "Server (" << Server::GetStatus() << "): " << Server::GetState();
    }
    #endif//TOKEN_ENABLE_SERVER
    return true;
  }
}