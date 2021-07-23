#ifndef TKN_NODE_SERVER_H
#define TKN_NODE_SERVER_H

#include "server/server.h"
#include "node/node_session.h"

namespace token{
  namespace node{
    class Server : public ServerBase<Session>{
    protected:
      ObjectPoolPtr pool_;
      BlockChainPtr chain_;

      Session* CreateSession() const override{
        return new Session(GetLoop());
      }
    public:
      Server(uv_loop_t* loop,
          const ObjectPoolPtr& pool,
          const BlockChainPtr& chain):
          ServerBase(loop),
          pool_(pool),
          chain_(chain){}
      Server(const ObjectPoolPtr& pool, const BlockChainPtr& chain):
        Server(uv_loop_new(), pool, chain){}
      Server():
        Server(nullptr, nullptr){}
      ~Server() override = default;

      BlockChainPtr GetChain() const{
        return chain_;
      }

      ObjectPoolPtr GetPool() const{
        return pool_;
      }

      static inline bool
      IsEnabled(){
        return IsValidPort(GetPort());
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
  }
}


#endif//TKN_NODE_SERVER_H