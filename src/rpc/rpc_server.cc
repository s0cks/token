#include "rpc/rpc_server.h"

namespace token{
  LedgerServer* LedgerServer::GetInstance(){
    static LedgerServer instance;
    return &instance;
  }

  static ThreadId thread_;

  bool ServerThread::Start(){
    return ThreadStart(&thread_, LedgerServer::GetThreadName(), &HandleThread, (uword)LedgerServer::GetInstance());
  }

  bool ServerThread::Join(){
    return ThreadJoin(thread_);
  }

  void ServerThread::HandleThread(uword param){
    auto instance = (LedgerServer*)param;
    DLOG_SERVER_IF(WARNING, !instance->Run()) << "failed to run the server";
    pthread_exit(nullptr);
  }
}