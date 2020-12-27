#include "server.h"
#include "discovery.h"
#include "http/healthcheck_service.h"
#include "utils/crash_report.h"
#include "peer/peer_session_manager.h"

namespace Token{
    void SignalHandlers::HandleInterrupt(int signum){
        LOG(INFO) << "terminating the peer session manager....";
        if(!PeerSessionManager::Shutdown()){
            CrashReport::PrintNewCrashReport("Cannot shutdown PeerSessionManager");
            goto terminate;
        }

        LOG(INFO) << "terminating the server....";
        if(!Server::Stop()){
            CrashReport::PrintNewCrashReport("Cannot shutdown Server");
            goto terminate;
        }

        LOG(INFO) << "terminating the block discovery thread....";
        if(!BlockDiscoveryThread::Stop()){
            CrashReport::PrintNewCrashReport("Cannot shutdown BlockDiscoveryThread");
            goto terminate;
        }

        LOG(INFO) << "terminating the health check service....";
        if(!HealthCheckService::Stop()){
            CrashReport::PrintNewCrashReport("Cannot shutdown HealthCheckService");
            goto terminate;
        }
    terminate:
        exit(signum);
    }

    void SignalHandlers::HandleSegfault(int signum){
//        std::stringstream ss;
//        ss << "Segfault: " << signum;
//        CrashReport::PrintNewCrashReportAndExit(ss);
    }
}