#ifdef TOKEN_ENABLE_SERVER
  #include "server.h"
  #include "peer/peer_session_manager.h"
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
  //#include "http/service_health.h"
#endif//TOKEN_ENABLE_HEALTH_SERVICE

#ifdef TOKEN_ENABLE_REST_SERVICE
  #include "http/http_service_rest.h"
#endif//TOKEN_ENABLE_REST_SERVICE

namespace token{
  static inline void
  PrintFatalCrashReport(const std::string& cause){
    //TODO: implement
  }

  static inline void
  Terminate(int signum){
    // terminate the block miner
    //TODO: implement

#ifdef TOKEN_ENABLE_REST_SERVICE
    // terminate the rest service
    if(IsValidPort(FLAGS_service_port) && HttpRestService::IsServiceRunning()){
#ifdef TOKEN_DEBUG
      LOG(INFO) << "terminating the rest service....";
      if(HttpRestService::Shutdown()){
        HttpRestService::WaitForShutdown();
      } else{
        LOG(WARNING) << "cannot shutdown the rest service.";
      }
#endif//TOKEN_DEBUG
    }
#endif//TOKEN_ENABLE_REST_SERVICE

    // terminate the peers
    //TODO: implement

    // terminate the server
    //TODO: implement

    // terminate the health service
    //TODO: implement

#ifdef TOKEN_ENABLE_REST_SERVICE
    /*if(IsValidPort(FLAGS_service_port) && HttpRestService::IsServiceRunning()){
      LOG(INFO) << "terminating the controller service....";
      if(!HttpRestService::Shutdown()){
        PrintFatalCrashReport("Cannot shutdown the controller service thread.");
        goto terminate;
      }
    }*/
#endif//TOKEN_ENABLE_REST_SERVICE

/*#ifdef TOKEN_ENABLE_SERVER
    LOG(INFO) << "terminating the peer session manager....";
    if(!PeerSessionManager::Shutdown()){
      PrintFatalCrashReport("Cannot shutdown the peer session manager threads.");
      goto terminate;
    }

    if(IsValidPort(FLAGS_server_port) && Server::IsRunning()){
      LOG(INFO) << "terminating the rpc....";
      if(!Server::Shutdown()){
        PrintFatalCrashReport("Cannot shutdown the rpc thread.");
        goto terminate;
      }
    }
#endif//TOKEN_ENABLE_SERVER*/

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
    /*if(IsValidPort(FLAGS_healthcheck_port) && HttpHealthService::IsServiceRunning()){
      LOG(INFO) << "terminating the health check service....";
      if(!HttpHealthService::Shutdown()){
        PrintFatalCrashReport("Cannot shutdown the health check service");
        goto terminate;
      }
    }*/
#endif//TOKEN_ENABLE_HEALTH_SERVICE
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