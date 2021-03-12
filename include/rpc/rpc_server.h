#ifndef TOKEN_RPC_SERVER_H
#define TOKEN_RPC_SERVER_H

#include "server.h"
#include "rpc/rpc_session.h"

namespace token{
#define ENVIRONMENT_TOKEN_CALLBACK_ADDRESS "TOKEN_CALLBACK_ADDRESS"

  class ServerSession : public RpcSession{
    friend class LedgerServer;
   private:
#define DECLARE_HANDLER(Name) \
    void On##Name##Message(const std::shared_ptr<Name##Message>& msg);
    FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLER)
#undef DECLARE_HANDLER
   public:
    ServerSession(uv_loop_t* loop):
      RpcSession(loop){}
    ~ServerSession() = default;

    void SendAccepted(){

    }

    void SendRejected(){

    }
  };

  class LedgerServer : public Server<RpcMessage>{
   protected:
    ServerSession* CreateSession() const{
      return new ServerSession(GetLoop());
    }
   public:
    LedgerServer(uv_loop_t* loop=uv_loop_new()):
      Server(loop, "rpc-server"){}
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