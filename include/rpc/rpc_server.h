#ifndef TOKEN_RPC_SERVER_H
#define TOKEN_RPC_SERVER_H

#include "server.h"
#include "rpc/rpc_session.h"

namespace token{
#define ENVIRONMENT_TOKEN_CALLBACK_ADDRESS "TOKEN_CALLBACK_ADDRESS"

  class ServerSession : public RpcSession{
   protected:
    bool ItemExists(const InventoryItem& item) const{
#ifdef TOKEN_DEBUG
      SESSION_LOG(INFO, this) << "searching for " << item << "....";
#endif//TOKEN_DEBUG
      Hash hash = item.GetHash();
      switch(item.GetType()){
        case Type::kBlock:{
          if(GetChain()->HasBlock(hash)){
#ifdef TOKEN_DEBUG
            SESSION_LOG(INFO, this) << item << " was found in the chain.";
#endif//TOKEN_DEBUG
            return true;
          }

          if(GetPool()->HasBlock(hash)){
#ifdef TOKEN_DEBUG
            SESSION_LOG(INFO, this) << item << " was found in the pool.";
#endif//TOKEN_DEBUG
            return true;
          }
          return false;
        }
        case Type::kTransaction:{
          if(GetPool()->HasTransaction(hash)){
#ifdef TOKEN_DEBUG
            SESSION_LOG(INFO, this) << item << " was found in the pool.";
#endif//TOKEN_DEBUG
            return true;
          }
          return false;
        }
        default:// should never happen
          return false;
      }
    }
   public:
    ObjectPoolPtr pool_;
    BlockChainPtr chain_;

    ServerSession(const ObjectPoolPtr& pool,
                  const BlockChainPtr& chain):
      RpcSession(),
      pool_(pool),
      chain_(chain){}
    ServerSession(uv_loop_t* loop, const ObjectPoolPtr& pool, const BlockChainPtr& chain):
      RpcSession(loop),
      pool_(pool),
      chain_(chain){}
    ~ServerSession() = default;

    ObjectPoolPtr GetPool() const{
      return pool_;
    }

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
    ObjectPoolPtr pool_;
    BlockChainPtr chain_;

    ServerSession* CreateSession() const{
      return new ServerSession(GetLoop(), GetPool(), GetChain());
    }
   public:
    LedgerServer(uv_loop_t* loop=uv_loop_new(), const BlockChainPtr& chain=BlockChain::GetInstance()):
      Server(loop, "rpc-server"),
      chain_(chain){}
    ~LedgerServer() = default;

    BlockChainPtr GetChain() const{
      return chain_;
    }

    ObjectPoolPtr GetPool() const{
      return pool_;
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