#ifndef TKN_SERVER_H
#define TKN_SERVER_H

namespace token{
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

#endif//TKN_SERVER_H