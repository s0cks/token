#ifndef TOKEN_RPC_SERVER_H
#define TOKEN_RPC_SERVER_H

#include "server/server.h"
#include "server/session.h"

namespace Token{
#define ENVIRONMENT_TOKEN_CALLBACK_ADDRESS "TOKEN_CALLBACK_ADDRESS"

  class LedgerServer : public Server<RpcMessage, ServerSession>{
   public:
    LedgerServer(uv_loop_t* loop=uv_loop_new()):
      Server(loop, "rpc"){}
    ~LedgerServer() = default;

    ServerPort GetPort() const{
      return FLAGS_server_port;
    }

    static bool Start();
    static bool Shutdown();
    static bool WaitForShutdown();

#define DECLARE_CHECK(Name) \
    static bool IsServer##Name();
    FOR_EACH_SERVER_STATE(DECLARE_CHECK)
#undef DECLARE_CHECK

    static inline NodeAddress
    GetCallbackAddress(){
      return NodeAddress::ResolveAddress(GetEnvironmentVariable(ENVIRONMENT_TOKEN_CALLBACK_ADDRESS));
    }
  };
}

#endif//TOKEN_RPC_SERVER_H