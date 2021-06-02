#include "http/test_http_controller_wallet.h"

namespace token{
  namespace http{
    static inline http::RequestPtr
    CreateGetUserWalletRequest(const http::SessionPtr& session, const User& user){
      std::string username = user.str();
      http::ParameterMap params;
      params.insert({ "user", username });
      return CreateRequestFor(session, http::Method::kGet, "/wallets/" + username, params);
    }

    static inline std::string
    GetResponseBodyForUserWallet(const User& user, const Wallet& wallet){
      json::String body;
      json::Writer writer(body);
      LOG_IF(ERROR, !writer.StartObject()) << "cannot start json object.";
      LOG_IF(ERROR, !writer.Key("data")) << "cannot set data field in json object.";
      LOG_IF(ERROR, !writer.StartObject()) << "cannot start json object.";

      LOG_IF(ERROR, !json::SetField(writer, "user", user)) << "cannot set user field in json object.";

      LOG_IF(ERROR, !writer.Key("wallet")) << "cannot set wallet field in json object.";
      LOG_IF(ERROR, !writer.StartArray()) << "cannot start json array.";
      for(auto& it : wallet){
        std::string hex = it.HexString();
        LOG_IF(ERROR, !writer.String(hex.data(), hex.length())) << "cannot append hash to json array.";
      }
      LOG_IF(ERROR, !writer.EndArray()) << "cannot end json array.";
      LOG_IF(ERROR, !writer.EndObject()) << "cannot end json object.";
      LOG_IF(ERROR, !writer.EndObject()) << "cannot end json object.";
      return std::string(body.GetString(), body.GetSize());
    }

    TEST_F(WalletControllerTest, TestGetUserWallet){
      SessionPtr session = NewMockHttpSession();
      WalletManagerPtr wallets = NewMockWalletManager();
      WalletController controller(wallets);

      User user("TestUser");
      Wallet wallet = {
          Hash::FromHexString("4338383533313133393832343834453131303642353645374643343230433836"),
          Hash::FromHexString("3731333443443738303435304139434646303643324130463636413033364530"),
      };
      EXPECT_CALL((MockWalletManager&)*wallets, HasWallet(IsUser(user)))
          .Times(testing::AtLeast(1))
          .WillRepeatedly(testing::Return(true));
      EXPECT_CALL((MockWalletManager&)*wallets, GetUserWallet(IsUser(user)))
          .Times(testing::AtLeast(1))
          .WillRepeatedly(testing::Return(wallet));

      EXPECT_CALL((MockHttpSession&)*session, Send(::testing::AllOf(ResponseIsOk(), ResponseBodyEqualsString(GetResponseBodyForUserWallet(user, wallet)))))
          .Times(::testing::AtLeast(1));
      controller.OnGetUserWallet(session, CreateGetUserWalletRequest(session, user));
    }

    TEST_F(WalletControllerTest, TestGetUserWalletTokenCode){
      NOT_IMPLEMENTED(WARNING);
    }

    TEST_F(WalletControllerTest, TestPostUserWalletSpend){
      NOT_IMPLEMENTED(WARNING);
    }
  }
}