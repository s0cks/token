#include "server.h"
#include "http/http_service_rest.h"
#include "http/http_service_health.h"
#include "peer/peer_session_manager.h"


namespace token{
  static inline void
  PrintFatalCrashReport(const std::string& cause){
    NOT_IMPLEMENTED(ERROR);//TODO: implement
  }

  static inline void
  Terminate(int signum){
    NOT_IMPLEMENTED(ERROR);//TODO: implement
  }

  void SignalHandlers::HandleInterrupt(int signum){
    Terminate(signum);
  }

  void SignalHandlers::HandleSegfault(int signum){
    PrintFatalCrashReport("Segfault");
    exit(signum);
  }
}