#ifndef TOKEN_TEST_RPC_SERVER_SESSION_H
#define TOKEN_TEST_RPC_SERVER_SESSION_H

#include "test_suite.h"
#include "rpc/rpc_server.h"
#include "rpc/mock_rpc_server_session.h"

namespace token{
  class ServerSessionTest : public ::testing::Test{
   public:
    ServerSessionTest():
      ::testing::Test(){}
    ~ServerSessionTest() = default;
  };
}

#endif//TOKEN_TEST_RPC_SERVER_SESSION_H