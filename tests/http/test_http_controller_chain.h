#ifndef TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H
#define TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H

#include "mock/mock_blockchain.h"
#include "http/test_http_controller.h"
#include "http/http_controller_chain.h"

namespace token{
  class ChainControllerTest : public HttpControllerTest{
   protected:
    ChainController controller_;
    MockBlockChain chain_;
   public:
    ChainControllerTest():
      HttpControllerTest(),
      controller_(&chain_),
      chain_(){}
    ~ChainControllerTest() = default;
  };
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H