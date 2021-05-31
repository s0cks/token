#include "http/test_http_controller_wallet.h"

namespace token{
  static inline http::RequestPtr
  CreateGetUserWalletRequest(const http::SessionPtr& session, const std::string& user){
    http::ParameterMap params;
    params.insert({ "user", user });
    return CreateRequestFor(session, http::Method::kGet, "/wallets/" + user, params);
  }

  TEST_F(WalletControllerTest, TestGetUserWallet){
    http::SessionPtr session = NewMockHttpSession();

    Wallet wallet = {
      Hash::FromHexString("4338383533313133393832343834453131303642353645374643343230433836"),
      Hash::FromHexString("3731333443443738303435304139434646303643324130463636413033364530"),
    };

    EXPECT_CALL((MockWalletManager&)*wallets_, HasWallet(IsUser("TestUser")))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(true));
    EXPECT_CALL((MockWalletManager&)*wallets_, GetUserWallet(IsUser("TestUser")))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(wallet));

    EXPECT_CALL((MockHttpSession&)*session, Send(::testing::AllOf(ResponseIsOk(), ResponseBodyEqualsString("{\"data\":{\"user\":\"TestUser\",\"wallet\":[\"3731333443443738303435304139434646303643324130463636413033364530\",\"4338383533313133393832343834453131303642353645374643343230433836\"]}}"))))
      .Times(::testing::AtLeast(1));

    GetController()->OnGetUserWallet(session, CreateGetUserWalletRequest(session, "TestUser"));
  }

  TEST_F(WalletControllerTest, TestGetUserWalletTokenCode){
    NOT_IMPLEMENTED(WARNING);
  }

  TEST_F(WalletControllerTest, TestPostUserWalletSpend){
    NOT_IMPLEMENTED(WARNING);
  }
}