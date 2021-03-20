#ifndef TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H
#define TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H

#include "mock/mock_blockchain.h"
#include "http/test_http_controller.h"
#include "http/http_controller_chain.h"

namespace token{
  class ChainControllerTest : public HttpControllerTest{
   protected:
    std::shared_ptr<MockBlockChain> chain_;
    std::shared_ptr<ChainController> controller_;

    inline std::shared_ptr<MockBlockChain>
    GetChain() const{
      return chain_;
    }

    inline std::shared_ptr<ChainController>
    GetController() const{
      return controller_;
    }
   public:
    ChainControllerTest():
      HttpControllerTest(),
      chain_(std::make_shared<MockBlockChain>()),
      controller_(std::make_shared<ChainController>(chain_)){}
    ~ChainControllerTest() = default;
  };
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_CHAIN_H