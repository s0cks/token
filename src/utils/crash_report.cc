#include <glog/logging.h>
#include "pool.h"
#include "wallet.h"
#include "version.h"
#include "blockchain.h"
#include "utils/printer.h"
#include "utils/crash_report.h"

#ifdef TOKEN_ENABLE_SERVER
  #include "server/server.h"
  #include "peer/peer_session_manager.h"
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_REST_SERVICE
  #include "http/rest/rest_service.h"
#endif//TOKEN_ENABLE_REST_SERVICE

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
  #include "http/health/health_service.h"
#endif//TOKEN_ENABLE_HEALTH_SERVICE

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
    return BannerPrinter::PrintBanner(this, 75);
  }

  bool CrashReportPrinter::PrintHostInformation(){
#ifdef OS_IS_LINUX
  #ifdef ARCHITECTURE_IS_X64
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: Linux (x64)";
  #elif ARCHITECTURE_IS_X32
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: Linux (x32)";
  #else
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: Linux (Unsupported)";
  #endif
#elif OS_IS_WINDOWS
  #if ARARCHITECTURE_IS_X64
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: Windows (x64)";
  #elif ARARCHITECTURE_IS_X32
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: Windows (x32)";
  #else
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: Windows (Unsupported)";
  #endif
#elif OS_IS_OSX
  #if ARARCHITECTURE_IS_X64
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: OSX (x64)";
  #elif ARCHARCHITECTURE_IS_X32
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: OSX (x32)";
  #else
    LOG_AT_LEVEL(GetSeverity()) << "Operating System: OSX (Unsupported)";
  #endif
#endif
    return true;
  }

  bool CrashReportPrinter::PrintCrashInformation(){
    LOG_AT_LEVEL(GetSeverity()) << "Cause: " << GetCause();
    LOG_AT_LEVEL(GetSeverity()) << "Timestamp: " << FormatTimestampReadable(Clock::now());
#ifdef OS_IS_LINUX
    LOG_AT_LEVEL(GetSeverity()) << "Stack Trace:";
    int32_t offset = 4;
    int32_t depth = 20;
    StackTrace trace(offset, depth);
    for(int32_t idx = offset; idx < trace.GetNumberOfFrames(); idx++)
      LOG_AT_LEVEL(GetSeverity()) << "\t" << (idx + 1 - offset) << ": " << trace[idx];
#endif//OS_IS_LINUX
    return true;
  }

  bool CrashReportPrinter::PrintSystemInformation(){
    LOG_AT_LEVEL(GetSeverity()) << "Version: " << Version();
    LOG_AT_LEVEL(GetSeverity()) << "Home: " << TOKEN_BLOCKCHAIN_HOME;

    // Chain Information
    if(BlockChain::IsInitialized()){
      LOG_AT_LEVEL(GetSeverity()) << "Block Chain:";
      LOG_AT_LEVEL(GetSeverity()) << "\tGenesis: " << BlockChain::GetReference(BLOCKCHAIN_REFERENCE_GENESIS);
      LOG_AT_LEVEL(GetSeverity()) << "\tHead: " << BlockChain::GetReference(BLOCKCHAIN_REFERENCE_HEAD);
    } else{
      PrintState<BlockChain>("Chain");
    }

    // Object Pool Information
    if(ObjectPool::IsInitialized()){
      LOG_AT_LEVEL(GetSeverity()) << "Pool: ";
      LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Objects: " << ObjectPool::GetNumberOfObjects();
      LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Blocks: " << ObjectPool::GetNumberOfBlocks();
      LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Transactions: " << ObjectPool::GetNumberOfTransactions();
      LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Unclaimed Transactions: "
                                  << ObjectPool::GetNumberOfUnclaimedTransactions();
    } else{
      PrintState<ObjectPool>("Pool");
    }

    // Wallet Information
    if(WalletManager::IsInitialized() && WalletManager::IsOk()){
      LOG_AT_LEVEL(GetSeverity()) << "Wallet Manager (" << WalletManager::GetStatus() << "):";
      LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Wallets: " << WalletManager::GetNumberOfWallets();
    } else{
      PrintState<WalletManager>("Wallet Manager");
    }
    return true;
  }

#ifdef TOKEN_ENABLE_SERVER
  bool CrashReportPrinter::PrintServerInformation(){
    return PrintState<Server>("Server");
  }

  static inline std::string
  GetPeerInfo(PeerSession* session){
    std::stringstream ss;
    ss << session->GetID() << " (" << session->GetAddress() << ")";
    return ss.str();
  }

  static inline std::string
  GetPeerStatus(PeerSession* session){
    std::stringstream ss;
    ss << session->GetStatus() << " [" << session->GetStatus() << "]";
    return ss.str();
  }

  bool CrashReportPrinter::PrintPeerInformation(){
    PrintState<PeerSessionManager>("Peer Session Manager");

    std::set<UUID> peers;
    if(!PeerSessionManager::GetConnectedPeers(peers))
      return false;

    if(peers.empty())
      return true;

    LOG_AT_LEVEL(GetSeverity()) << "\tSessions:";
    for(auto& it : peers){
      PeerSession* session = PeerSessionManager::GetSession(it);
      LOG_AT_LEVEL(GetSeverity()) << "\t  - " << GetPeerInfo(session) << ": " << GetPeerStatus(session);
    }
    return true;
  }
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
  bool CrashReportPrinter::PrintHealthServiceInformation(){
    return PrintState<HealthCheckService>("Health Service");
  }
#endif//TOKEN_ENABLE_HEALTH_SERVICE

#ifdef TOKEN_ENABLE_REST_SERVICE
  bool CrashReportPrinter::PrintRestServiceInformation(){
    return PrintState<RestService>("Rest Service");
  }
#endif//TOKEN_ENABLE_REST_SERVICE
}