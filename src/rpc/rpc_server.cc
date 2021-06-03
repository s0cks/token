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
}