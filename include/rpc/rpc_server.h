#ifndef TOKEN_RPC_SERVER_H
#define TOKEN_RPC_SERVER_H

#include "env.h"
#include "server.h"
#include "rpc/rpc_session.h"

namespace token{
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

  class ServerSession : public RpcSession{
   protected:
    bool ItemExists(const InventoryItem& item) const{
      DLOG_SESSION(INFO, this) << "searching for " << item << "....";
      Hash hash = item.GetHash();
      switch(item.GetType()){
        case Type::kBlock:{
          if(GetChain()->HasBlock(hash)){
            DLOG_SESSION(INFO, this) << item << " was found in the chain.";
            return true;
          }

          if(GetPool()->HasBlock(hash)){
            DLOG_SESSION(INFO, this) << item << " was found in the pool.";
            return true;
          }
          return false;
        }
        case Type::kTransaction:{
          if(GetPool()->HasTransaction(hash)){
            DLOG_SESSION(INFO, this) << item << " was found in the pool.";
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
      Server(loop, GetThreadName()),
      chain_(chain){}
    ~LedgerServer() = default;

    BlockChainPtr GetChain() const{
      return chain_;
    }

    ObjectPoolPtr GetPool() const{
      return pool_;
    }

    ServerPort GetPort() const{
      return GetServerPort();
    }

    static inline ServerPort
    GetServerPort(){
      return FLAGS_server_port;
    }

    static const char*
    GetThreadName(){
      return "server";
    }

    static LedgerServer* GetInstance();
  };

  class ServerThread{
   private:
    static void HandleThread(uword param);
   public:
    ServerThread() = delete;
    ~ServerThread() = delete;

    static bool Join();
    static bool Start();
  };
}

#endif//TOKEN_RPC_SERVER_H