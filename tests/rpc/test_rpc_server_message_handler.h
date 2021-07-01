#ifndef TOKEN_TEST_RPC_SERVER_MESSAGE_HANDLER_H
#define TOKEN_TEST_RPC_SERVER_MESSAGE_HANDLER_H

#include "test_suite.h"
#include "network/rpc_server_session.h"
#include "rpc/mock_rpc_server_session.h"

namespace token{
  namespace rpc{
    class ServerMessageHandlerTest: public ::testing::Test{
    public:
      ServerMessageHandlerTest():
         ::testing::Test(){}
      ~ServerMessageHandlerTest() override = default;
    };
  }
}
#endif //TOKEN_TEST_RPC_SERVER_MESSAGE_HANDLER_H
