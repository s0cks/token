#include "server/rpc/rpc_server.h"

namespace Token{
  LedgerServer* LedgerServer::GetInstance(){
    static LedgerServer server;
    return &server;
  }
}