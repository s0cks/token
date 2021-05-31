#ifndef TOKEN_TEST_HTTP_CONTROLLER_WALLET_H
#define TOKEN_TEST_HTTP_CONTROLLER_WALLET_H

#include "mock/mock_wallet_manager.h"
#include "http/test_http_controller.h"
#include "http/http_controller_wallet.h"

namespace token{
   class WalletControllerTest : public http::ControllerTest<http::WalletController>{
    protected:
     std::shared_ptr<WalletManager> wallets_;
    public:
     WalletControllerTest():
      http::ControllerTest<http::WalletController>(),
      wallets_(std::make_shared<MockWalletManager>()){}
    ~WalletControllerTest() override = default;

     std::shared_ptr<WalletManager> GetWalletManager() const{
       return wallets_;
     }
   };
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_WALLET_H