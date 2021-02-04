#include "rpc/rpc_server.h"

namespace token{
  static LedgerServer instance;

  bool LedgerServer::Start(){
    return instance.StartThread();
  }

  bool LedgerServer::Shutdown(){
    return false; //TODO: implement
  }

  bool LedgerServer::WaitForShutdown(){
    return instance.JoinThread();
  }

#define DEFINE_CHECK(Name) \
  bool LedgerServer::IsServer##Name(){ return instance.GetState() == Server::k##Name##State; }
  FOR_EACH_SERVER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK
}