#ifndef TOKEN_RPC_SERVER_H
#define TOKEN_RPC_SERVER_H

#include "env.h"
#include "pool.h"
#include "server.h"
#include "inventory.h"
#include "blockchain.h"
#include "rpc/rpc_common.h"
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

  namespace rpc{
  class ServerSession;
  typedef std::shared_ptr<ServerSession> ServerSessionPtr;

  class ServerMessageHandler : public rpc::MessageHandler{
    friend class ServerSession;
   public:
    explicit ServerMessageHandler(ServerSession* session):
       rpc::MessageHandler((rpc::Session*)session){}
    ~ServerMessageHandler() override = default;

    ObjectPoolPtr GetPool() const;
    BlockChainPtr GetChain() const;

#define DECLARE_HANDLE(Name) \
    void On##Name##Message(const Name##MessagePtr& msg) override;
    FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLE)
#undef DECLARE_HANDLE
  };

  class ServerSession : public rpc::Session{
   private:
    ObjectPoolPtr pool_;
    BlockChainPtr chain_;
    ServerMessageHandler handler_;
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

    void OnMessageRead(const BufferPtr& buff) override{
      rpc::MessagePtr msg = rpc::Message::From(buff);
      switch(msg->GetType()) {
#define DEFINE_HANDLE(Name) \
        case Type::k##Name##Message: \
          return GetMessageHandler().On##Name##Message(std::static_pointer_cast<rpc::Name##Message>(msg));

        FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLE)
#undef DEFINE_HANDLE
        default: return;
      }
    }
   public:
    ServerSession(ObjectPoolPtr  pool,
                  BlockChainPtr  chain):
       rpc::Session(),
       pool_(std::move(pool)),
       chain_(std::move(chain)),
       handler_(this){}
    ServerSession(uv_loop_t* loop,
                  ObjectPoolPtr pool,
                  const BlockChainPtr& chain):
       rpc::Session(loop),
       pool_(std::move(pool)),
       chain_(chain),
       handler_(this){}
    ~ServerSession() override = default;

    rpc::MessageHandler& GetMessageHandler() override{
      return handler_;
    }

    ObjectPoolPtr GetPool() const{
      return pool_;
    }

    BlockChainPtr GetChain() const{
      return chain_;
    }

    bool SendPrepare() override{
      NOT_IMPLEMENTED(WARNING);
      return false;//TODO: implement
    }

    bool SendPromise() override{
      NOT_IMPLEMENTED(WARNING);
      return false;//TODO: implement
    }

    bool SendCommit() override{
      NOT_IMPLEMENTED(WARNING);
      return false;//TODO: implement
    }

    bool SendAccepted() override{
      NOT_IMPLEMENTED(WARNING);
      return false;//TODO: implement
    }

    bool SendRejected() override{
      NOT_IMPLEMENTED(WARNING);
      return false;//TODO: implement
    }
  };

  class LedgerServer : public ServerBase{
   protected:
    ObjectPoolPtr pool_;
    BlockChainPtr chain_;

    std::shared_ptr<SessionBase> CreateSession() const override{
      return std::make_shared<ServerSession>(GetLoop(), GetPool(), GetChain());
    }
   public:
    LedgerServer(uv_loop_t* loop=uv_loop_new(),
                 const ObjectPoolPtr& pool=ObjectPool::GetInstance(),
                 const BlockChainPtr& chain=BlockChain::GetInstance()):
      ServerBase(loop),
      pool_(pool),
      chain_(chain){}
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

    static inline LedgerServer* NewInstance(){
      return new LedgerServer();
    }

    static LedgerServer* GetInstance();
  };

  template<class Server>
  class ServerThread{
   protected:
    ThreadId thread_;

    static void HandleThread(uword param){
      auto instance = Server::NewInstance();
      LOG_THREAD_IF(ERROR, !instance->Run(Server::GetPort())) << "Failed to run the " << Server::GetName() << " server loop.";
      pthread_exit(nullptr);
    }
   public:
    ServerThread() = default;
    ~ServerThread() = default;

    bool Start(){
      return ThreadStart(&thread_, Server::GetName(), &HandleThread, (uword)0);
    }

    bool Join(){
      return ThreadJoin(thread_);
    }
  };

  class LedgerServerThread : public ServerThread<LedgerServer>{};
  }
}

#endif//TOKEN_RPC_SERVER_H