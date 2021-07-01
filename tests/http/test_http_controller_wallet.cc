#include "http/test_http_controller_wallet.h"

namespace token{
  namespace http{
    TEST_F(WalletControllerTest, TestGetUserWallet){
      SessionPtr session = NewMockHttpSession();
      WalletManagerPtr wallets = NewMockWalletManager();
      WalletController controller(wallets);

      Wallet wallet = {
          Hash::FromHexString("4338383533313133393832343834453131303642353645374643343230433836"),
          Hash::FromHexString("3731333443443738303435304139434646303643324130463636413033364530"),
      };

      ParameterMap params;
      params["user"] = "TestUser";
      RequestPtr request = NewGetRequest("/wallet/TestUser", params);

      ResponsePtr response = NewOkResponse(User("TestUser"), wallet);

      EXPECT_CALL((MockWalletManager&)*wallets, HasWallet(IsUser("TestUser")))
        .Times(testing::AtLeast(1))
        .WillRepeatedly(testing::Return(true));
      EXPECT_CALL((MockWalletManager&)*wallets, GetUserWallet(IsUser("TestUser")))
        .Times(testing::AtLeast(1))
        .WillRepeatedly(testing::Return(wallet));
      EXPECT_CALL((MockHttpSession&)*session, Send(ResponseEquals(response)))
        .Times(::testing::AtLeast(1));
      controller.OnGetUserWallet(session, request);
    }

    TEST_F(WalletControllerTest, TestGetUserWalletTokenCode){
      NOT_IMPLEMENTED(WARNING);
    }

    TEST_F(WalletControllerTest, TestPostUserWalletSpend){
      NOT_IMPLEMENTED(WARNING);
    }
  }
}