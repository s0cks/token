#ifndef TOKEN_TEST_HTTP_CONTROLLER_H
#define TOKEN_TEST_HTTP_CONTROLLER_H

#include "test_suite.h"
#include "mocks/mock_blockchain.h"
#include "mocks/mock_http_session.h"
#include "http/http_controller_chain.h"

namespace token{
  class ChainControllerTest : public ::testing::Test{
   protected:
    MockBlockChain chain_;
    ChainController controller_;
   public:
    ChainControllerTest():
      ::testing::Test(),
      chain_(),
      controller_(&chain_){}
    ~ChainControllerTest() = default;
  };
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_H