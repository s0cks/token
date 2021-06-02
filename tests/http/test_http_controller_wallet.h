#ifndef TOKEN_TEST_HTTP_CONTROLLER_WALLET_H
#define TOKEN_TEST_HTTP_CONTROLLER_WALLET_H

#include "mock_wallet_manager.h"
#include "http/test_http_controller.h"
#include "http/http_controller_wallet.h"

namespace token{
  namespace http{
   class WalletControllerTest : public ::testing::Test{
    public:
     WalletControllerTest() = default;
     ~WalletControllerTest() override = default;
   };
  }
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_WALLET_H