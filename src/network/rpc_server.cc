#include "pool.h"
#include "blockchain.h"
#include "network/rpc_server.h"

namespace token{
  namespace rpc{
    std::unique_ptr<LedgerServer> LedgerServer::NewInstance(){
      return std::unique_ptr<LedgerServer>(new LedgerServer(ObjectPool::GetInstance(), BlockChain::GetInstance()));
    }
  }
}