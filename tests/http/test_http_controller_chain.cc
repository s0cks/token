#include "http/test_http_controller_chain.h"

namespace token{
  using testing::AtLeast;
  using testing::AtMost;
  using testing::Return;

  TEST_F(ChainControllerTest, TestGetBlockChainHead){
    BlockPtr head = Block::Genesis();
    EXPECT_CALL(*chain_, GetHead())
        .Times(AtLeast(1))
        .WillRepeatedly(Return(head));

    MockHttpSession session;
    EXPECT_CALL(session, Send(ResponseIs(HttpStatusCode::kHttpOk, "{\"data\":{\"timestamp\":0,\"version\":\"1.0.0\",\"height\":0,\"previous_hash\":\"0000000000000000000000000000000000000000000000000000000000000000\",\"hash\":\"A05C67771FB1A82002A7580CAAC5A1B8511B3985EB341319ECFF9FCCA75352EC\",\"transactions\":3}}")))
        .Times(AtLeast(1));
    GetController()->OnGetBlockChainHead(&session, nullptr);
  }

  TEST_F(ChainControllerTest, TestGetBlockChain){
    BlockPtr head = Block::Genesis();
    EXPECT_CALL(*chain_, GetHead())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(head));
    EXPECT_CALL(*chain_, GetBlock(IsHash(Hash())))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(nullptr));

    MockHttpSession session;
    EXPECT_CALL(session, Send(ResponseIs(HttpStatusCode::kHttpOk, "[\"A05C67771FB1A82002A7580CAAC5A1B8511B3985EB341319ECFF9FCCA75352EC\"]")))
        .Times(AtLeast(1));
    GetController()->OnGetBlockChain(&session, nullptr);
  }

  TEST_F(ChainControllerTest, TestGetBlockChainBlock){
    BlockPtr blk = Block::Genesis();
    Hash hash = blk->GetHash();
    EXPECT_CALL(*chain_, GetBlock(IsHash(hash)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(blk));
    EXPECT_CALL(*chain_, HasBlock(IsHash(hash)))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));

    MockHttpSession session;
    HttpRequestBuilder builder(&session);
    builder.SetMethod(HttpMethod::kGet);
    builder.SetPath("/chain/data/A05C67771FB1A82002A7580CAAC5A1B8511B3985EB341319ECFF9FCCA75352EC");
    builder.SetPathParameter("hash", "A05C67771FB1A82002A7580CAAC5A1B8511B3985EB341319ECFF9FCCA75352EC");

    EXPECT_CALL(session, Send(ResponseIs(HttpStatusCode::kHttpOk, "{\"data\":{\"timestamp\":0,\"version\":\"1.0.0\",\"height\":0,\"previous_hash\":\"0000000000000000000000000000000000000000000000000000000000000000\",\"hash\":\"A05C67771FB1A82002A7580CAAC5A1B8511B3985EB341319ECFF9FCCA75352EC\",\"transactions\":3}}")))
      .Times(AtLeast(1));
    GetController()->OnGetBlockChainBlock(&session, builder.Build());
  }
}