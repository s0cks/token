#ifndef TOKEN_MOCK_RPC_SERVER_SESSION_H
#define TOKEN_MOCK_RPC_SERVER_SESSION_H

#include <gmock/gmock.h>

#include "rpc/rpc_server.h"
#include "mock/mock_blockchain.h"
#include "mock/mock_object_pool.h"

namespace token{
  class MockServerSession;
  typedef std::shared_ptr<MockServerSession> MockServerSessionPtr;

  class MockServerSession : public rpc::ServerSession{
   public:
    MockServerSession(const ObjectPoolPtr& pool, const BlockChainPtr& chain):
      rpc::ServerSession(pool, chain){}
    ~MockServerSession() override = default;

    MOCK_METHOD(void, Send, (const rpc::MessagePtr&), ());
    MOCK_METHOD(void, SendMessages, (const rpc::MessageList&), ());

    static inline MockServerSessionPtr
    NewInstance(const MockBlockChainPtr& chain, const MockObjectPoolPtr& pool){
      return std::make_shared<MockServerSession>(pool, chain);
    }

    static inline MockServerSessionPtr
    NewInstance(){
      return NewInstance(MockBlockChain::NewInstance(), MockObjectPool::NewInstance());
    }
  };
}

#endif//TOKEN_MOCK_RPC_SERVER_SESSION_H