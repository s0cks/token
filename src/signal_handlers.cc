#include "discovery.h"
#include "utils/crash_report.h"

#ifdef TOKEN_ENABLE_SERVER
  #include "server.h"
  #include "peer/peer_session_manager.h"
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
  #include "http/healthcheck_service.h"
#endif//TOKEN_ENABLE_HEALTH_SERVICE

#ifdef TOKEN_ENABLE_REST_SERVICE
  #include "http/rest_service.h"
#endif//TOKEN_ENABLE_REST_SERVICE

namespace Token{
  static inline void
  PrintFatalCrashReport(const std::string& cause){
    CrashReportPrinter printer(cause, google::FATAL);
    if(!printer.Print())
      LOG(FATAL) << "couldn't print crash report.";
  }

  static inline void
  Terminate(int signum){
    #ifdef TOKEN_ENABLE_REST_SERVICE
      if(IsValidPort(FLAGS_service_port) && RestService::IsRunning()){
        LOG(INFO) << "terminating the rest service....";
        if(!RestService::Stop()){
          PrintFatalCrashReport("Cannot shutdown the rest service thread.");
          goto terminate;
        }
      }
    #endif//TOKEN_ENABLE_REST_SERVICE

    #ifdef TOKEN_ENABLE_SERVER
      LOG(INFO) << "terminating the peer session manager....";
      if(!PeerSessionManager::Shutdown()){
        PrintFatalCrashReport("Cannot shutdown the peer session manager threads.");
        goto terminate;
      }

      if(IsValidPort(FLAGS_server_port) && Server::IsRunning()){
        LOG(INFO) << "terminating the server....";
        if(!Server::Stop()){
          PrintFatalCrashReport("Cannot shutdown the server thread.");
          goto terminate;
        }
      }
    #endif//TOKEN_ENABLE_SERVER

    LOG(INFO) << "terminating the block discovery thread....";
    if(!BlockDiscoveryThread::Stop()){
      PrintFatalCrashReport("Cannot shutdown the block discovery thread.");
      goto terminate;
    }

    #ifdef TOKEN_ENABLE_HEALTH_SERVICE
      if(IsValidPort(FLAGS_healthcheck_port) && HealthCheckService::IsRunning()){
        LOG(INFO) << "terminating the health check service....";
        if(!HealthCheckService::Stop()){
          PrintFatalCrashReport("Cannot shutdown the health check service");
          goto terminate;
        }
      }
    #endif//TOKEN_ENABLE_HEALTH_SERVICE
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