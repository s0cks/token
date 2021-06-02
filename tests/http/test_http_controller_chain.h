#ifndef TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H
#define TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H

#include "mock_blockchain.h"
#include "http/test_http_controller.h"
#include "http/http_controller_chain.h"

namespace token{
  namespace http{
    class ChainControllerTest : public ::testing::Test{
     public:
      ChainControllerTest() = default;
      ~ChainControllerTest() override = default;
    };
  }
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H