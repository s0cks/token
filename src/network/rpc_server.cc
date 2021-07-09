#include "pool.h"
#include "blockchain.h"
#include "network/rpc_server.h"

namespace token{
  namespace rpc{
    LedgerServer::LedgerServer():
      LedgerServer(uv_loop_new(), ObjectPool::GetInstance(), BlockChain::GetInstance()){}
  }
}