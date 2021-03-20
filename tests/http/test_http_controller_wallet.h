#ifndef TOKEN_TEST_HTTP_CONTROLLER_WALLET_H
#define TOKEN_TEST_HTTP_CONTROLLER_WALLET_H

#include "mock/mock_wallet_manager.h"
#include "http/test_http_controller.h"
#include "http/http_controller_wallet.h"

namespace token{
  class WalletControllerTest : public HttpControllerTest{
   protected:
    WalletController controller_;
    MockWalletManager wallets_;
   public:
    WalletControllerTest():
      HttpControllerTest(),
      controller_(&wallets_),
      wallets_(){}
    ~WalletControllerTest() = default;
  };
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_WALLET_H