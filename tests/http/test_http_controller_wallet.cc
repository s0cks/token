#include "http/test_http_controller_wallet.h"

namespace token{
  static inline HttpRequestPtr
  CreateGetUserWalletRequest(MockHttpSession* session, const std::string& user){
    std::string path = "/wallets/" + user;

    ParameterMap params;
    params.insert({ "user", user });
    return CreateRequestFor(session, HttpMethod::kGet, path, params);
  }

  TEST_F(WalletControllerTest, TestGetUserWallet){
    Wallet wallet = {
      Hash::FromHexString("4338383533313133393832343834453131303642353645374643343230433836"),
      Hash::FromHexString("3731333443443738303435304139434646303643324130463636413033364530"),
    };

    EXPECT_CALL(wallets_, HasWallet(IsUser("TestUser")))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(wallets_, GetUserWallet(IsUser("TestUser")))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(wallet));

    MockHttpSession session;
    EXPECT_CALL(session, Send(ResponseIs(HttpStatusCode::kHttpOk, "{\"data\":{\"user\":\"TestUser\",\"wallet\":[\"3731333443443738303435304139434646303643324130463636413033364530\",\"4338383533313133393832343834453131303642353645374643343230433836\"]}}")));
    controller_.OnGetUserWallet(&session, CreateGetUserWalletRequest(&session, "TestUser"));
  }

  TEST_F(WalletControllerTest, TestGetUserWalletTokenCode){
    NOT_IMPLEMENTED(WARNING);
  }

  TEST_F(WalletControllerTest, TestPostUserWalletSpend){
    NOT_IMPLEMENTED(WARNING);
  }
}