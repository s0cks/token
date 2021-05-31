#ifndef TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H
#define TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H

#include "mock/mock_blockchain.h"
#include "http/test_http_controller.h"
#include "http/http_controller_chain.h"

namespace token{
  class ChainControllerTest : public http::ControllerTest<http::ChainController>{
   public:
    ChainControllerTest():
      http::ControllerTest<http::ChainController>(http::ChainController::NewInstance(MockBlockChain::NewInstance())){}
    ~ChainControllerTest() override = default;

    BlockChainPtr GetChain() const{
      return GetController()->GetChain();
    }
  };
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H