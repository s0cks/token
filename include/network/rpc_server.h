#ifndef TOKEN_RPC_SERVER_H
#define TOKEN_RPC_SERVER_H

#include <memory>

#include "env.h"
#include "server.h"
#include "network/rpc_server_session.h"

namespace token{
  namespace env{
#define TOKEN_ENVIRONMENT_VARIABLE_SERVER_CALLBACK_ADDRESS "TOKEN_CALLBACK_ADDRESS"

    static inline NodeAddress
    GetDefaultServerCallbackAddress(){
      std::stringstream ss;
      ss << "localhost:" << FLAGS_server_port;
      return NodeAddress::ResolveAddress(ss.str());
    }

    static inline NodeAddress
    GetServerCallbackAddress(){
      if(!env::HasVariable(TOKEN_ENVIRONMENT_VARIABLE_SERVER_CALLBACK_ADDRESS))
        return GetDefaultServerCallbackAddress();
      return NodeAddress::ResolveAddress(env::GetString(TOKEN_ENVIRONMENT_VARIABLE_SERVER_CALLBACK_ADDRESS));
    }
  }

  namespace rpc{
   class LedgerServer : public ServerBase<rpc::ServerSession>{
   protected:
    ObjectPoolPtr pool_;
    BlockChainPtr chain_;

    ServerSession* CreateSession() const override{
      return new ServerSession(GetLoop(), GetPool(), GetChain());
    }
   public:
    LedgerServer(uv_loop_t* loop,
                 const ObjectPoolPtr& pool,
                 const BlockChainPtr& chain):
      ServerBase(loop),
      pool_(pool),
      chain_(chain){}
    LedgerServer(const ObjectPoolPtr& pool, const BlockChainPtr& chain):
      LedgerServer(uv_loop_new(), pool, chain){}
    LedgerServer();
    ~LedgerServer() override = default;

    BlockChainPtr GetChain() const{
      return chain_;
    }

    ObjectPoolPtr GetPool() const{
      return pool_;
    }

    static inline bool
    IsEnabled(){
      return IsValidPort(LedgerServer::GetPort());
    }

    static inline ServerPort
    GetPort(){
      return FLAGS_server_port;
    }

    static inline const char*
    GetName(){
      return "rpc/server";
    }
  };

  class LedgerServerThread : public ServerThread<LedgerServer>{};
  }
}

#endif//TOKEN_RPC_SERVER_H