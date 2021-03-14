#include "test_http_controller.h"

namespace token{
  using testing::AtLeast;
  using testing::AtMost;

  MATCHER_P(ResponseIs, response, "doesn't match"){
    LOG(INFO) << arg->ToString();
    return true;
  }

  TEST_F(ChainControllerTest, TestGetBlockChainHead){
    EXPECT_CALL(chain_, GetHead())
      .Times(AtLeast(1));

    MockHttpSession session;
    EXPECT_CALL(session, Send(ResponseIs("")))
      .Times(AtLeast(1));

    controller_.OnGetBlockChainHead(&session, nullptr);
  }
}