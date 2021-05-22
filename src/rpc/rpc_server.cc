#include "rpc/rpc_server.h"

namespace token{
  namespace rpc{
    LedgerServer* LedgerServer::GetInstance(){
      static LedgerServer instance;
      return &instance;
    }

    BlockChainPtr ServerMessageHandler::GetChain() const{
      return ((ServerSession*)GetSession())->GetChain();
    }

    ObjectPoolPtr ServerMessageHandler::GetPool() const{
      return ((ServerSession*)GetSession())->GetPool();
    }
  }

  static ThreadId thread_;

  bool ServerThread::Start(){
    return ThreadStart(
            &thread_,
            rpc::LedgerServer::GetThreadName(),
            &HandleThread,
            (uword)rpc::LedgerServer::GetInstance());
  }

  bool ServerThread::Join(){
    return ThreadJoin(thread_);
  }

  void ServerThread::HandleThread(uword param){
    auto instance = (rpc::LedgerServer*)param;
    DLOG_SERVER_IF(WARNING, !instance->Run()) << "failed to run the server";
    pthread_exit(nullptr);
  }
}