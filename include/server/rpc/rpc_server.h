#ifndef TOKEN_RPC_SERVER_H
#define TOKEN_RPC_SERVER_H

#include "server/server.h"

namespace Token{
#define ENVIRONMENT_TOKEN_CALLBACK_ADDRESS "TOKEN_CALLBACK_ADDRESS"

  class LedgerServer : public Server{
   private:
    LedgerServer():
      Server(uv_loop_new()){}
   public:
    ~LedgerServer() = default;
    static LedgerServer* GetInstance();

    static inline UUID
    GetID(){
      return BlockChainConfiguration::GetSererID();
    }

    static inline NodeAddress
    GetCallbackAddress(){
      return NodeAddress::ResolveAddress(GetEnvironmentVariable(ENVIRONMENT_TOKEN_CALLBACK_ADDRESS));
    }
  };
}

#endif//TOKEN_RPC_SERVER_H