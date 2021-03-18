#ifndef TOKEN_RPC_SERVER_H
#define TOKEN_RPC_SERVER_H

#include "server.h"
#include "rpc/rpc_session.h"

namespace token{
#define ENVIRONMENT_TOKEN_CALLBACK_ADDRESS "TOKEN_CALLBACK_ADDRESS"

  class ServerSession : public RpcSession{
   public:
    BlockChainPtr chain_;

    ServerSession(const BlockChainPtr& chain):
      RpcSession(),
      chain_(chain){}
    ServerSession(uv_loop_t* loop, const BlockChainPtr& chain):
      RpcSession(loop),
      chain_(chain){}
    ~ServerSession() = default;

    BlockChainPtr GetChain() const{
      return chain_;
    }

#define DECLARE_HANDLER(Name) \
    virtual void On##Name##Message(const std::shared_ptr<Name##Message>& msg);
    FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLER)
#undef DECLARE_HANDLER

    bool SendPrepare(){
      NOT_IMPLEMENTED(WARNING);
      return false;//TODO: implement
    }

    bool SendPromise(){
      NOT_IMPLEMENTED(WARNING);
      return false;//TODO: implement
    }

    bool SendCommit(){
      NOT_IMPLEMENTED(WARNING);
      return false;//TODO: implement
    }

    bool SendAccepted(){
      NOT_IMPLEMENTED(WARNING);
      return false;//TODO: implement
    }

    bool SendRejected(){
      NOT_IMPLEMENTED(WARNING);
      return false;//TODO: implement
    }
  };

  class LedgerServer : public Server<RpcMessage>{
   protected:
    BlockChainPtr chain_;

    ServerSession* CreateSession() const{
      return new ServerSession(GetLoop(), GetChain());
    }
   public:
    LedgerServer(uv_loop_t* loop=uv_loop_new(), const BlockChainPtr& chain=BlockChain::GetInstance()):
      Server(loop, "rpc-server"),
      chain_(chain){}
    ~LedgerServer() = default;

    BlockChainPtr GetChain() const{
      return chain_;
    }

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