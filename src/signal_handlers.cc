#include "server.h"
#include "discovery.h"
#include "http/healthcheck_service.h"
#include "utils/crash_report.h"
#include "peer/peer_session_manager.h"

namespace Token{
  static inline void
  PrintFatalCrashReport(const std::string& cause){
    CrashReportPrinter printer(cause, google::FATAL);
    if(!printer.Print())
      LOG(FATAL) << "couldn't print crash report.";
  }

  static inline void
  Terminate(int signum){
    LOG(INFO) << "terminating the peer session manager....";
    if(!PeerSessionManager::Shutdown()){
      PrintFatalCrashReport("Cannot shutdown the peer session manager threads.");
      goto terminate;
    }

    LOG(INFO) << "terminating the server....";
    if(Server::IsRunning() && !Server::Stop()){
      PrintFatalCrashReport("Cannot shutdown the server thread.");
      goto terminate;
    }

    LOG(INFO) << "terminating the block discovery thread....";
    if(!BlockDiscoveryThread::Stop()){
      PrintFatalCrashReport("Cannot shutdown the block discovery thread.");
      goto terminate;
    }

    LOG(INFO) << "terminating the health check service....";
    if(!HealthCheckService::Stop()){
      PrintFatalCrashReport("Cannot shutdown the health check service");
      goto terminate;
    }
  terminate:
    exit(signum);
  }

  void SignalHandlers::HandleInterrupt(int signum){
    Terminate(signum);
  }

  void SignalHandlers::HandleSegfault(int signum){
    PrintFatalCrashReport("Segfault");
    exit(signum);
  }
}