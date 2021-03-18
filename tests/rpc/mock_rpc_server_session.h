#ifndef TOKEN_MOCK_RPC_SERVER_SESSION_H
#define TOKEN_MOCK_RPC_SERVER_SESSION_H

#include <gmock/gmock.h>
#include "rpc/rpc_server.h"
#include "mock/mock_blockchain.h"

namespace token{
  class MockServerSession : public ServerSession{
   public:
    MockServerSession(MockBlockChain* chain):
      ServerSession(chain){}
    ~MockServerSession() = default;

    MOCK_METHOD(void, Send, (const RpcMessagePtr&), ());
    MOCK_METHOD(void, SendMessages, (const RpcMessageList&), ());
  };
}

#endif//TOKEN_MOCK_RPC_SERVER_SESSION_H