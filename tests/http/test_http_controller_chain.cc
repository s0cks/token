#include "json.h"
#include "http/test_http_controller_chain.h"

namespace token{
  namespace http{
    TEST_F(ChainControllerTest, TestGetBlockChainHead){
      SessionPtr session = NewMockHttpSession();
      ::testing::Mock::AllowLeak(session.get()); // not a leak, this is a shared ptr

      BlockChainPtr chain = NewMockBlockChain();
      ::testing::Mock::AllowLeak(session.get()); // not a leak, this is a shared ptr

      ChainController controller(chain);

      BlockPtr head = Block::Genesis();

      RequestPtr request = NewGetRequest(session, "/chain/head");
      ResponsePtr response = NewOkResponse(session, head);

      EXPECT_CALL((MockBlockChain&)*chain, GetHead())
          .Times(::testing::AtLeast(1))
          .WillRepeatedly(::testing::Return(head));
      EXPECT_CALL((MockHttpSession&)*session, Send(ResponseEquals(response)))
          .Times(::testing::AtLeast(1));
      controller.OnGetBlockChainHead(session, request);
    }

    TEST_F(ChainControllerTest, TestGetBlockChain){
      SessionPtr session = NewMockHttpSession();
      ::testing::Mock::AllowLeak(session.get()); // not a leak, this is a shared ptr

      BlockChainPtr chain = NewMockBlockChain();
      ::testing::Mock::AllowLeak(session.get()); // not a leak, this is a shared ptr

      ChainController controller(chain);

      BlockPtr head = Block::Genesis();

      RequestPtr request = NewGetRequest(session, "/chain");
      ResponsePtr response = NewOkResponse(session, HashList{ head->GetHash() });

      EXPECT_CALL((MockBlockChain&)*chain, GetHead())
          .Times(::testing::AtLeast(1))
          .WillRepeatedly(::testing::Return(head));
      EXPECT_CALL((MockHttpSession&)*session, Send(ResponseEquals(response)))
          .Times(::testing::AtLeast(1));
      controller.OnGetBlockChain(session, request);
    }

    TEST_F(ChainControllerTest, TestGetBlockChainBlock){
      SessionPtr session = NewMockHttpSession();
      ::testing::Mock::AllowLeak(session.get()); // not a leak, this is a shared ptr

      BlockChainPtr chain = NewMockBlockChain();
      ChainController controller(chain);

      BlockPtr head = Block::Genesis();
      Hash head_hash = head->GetHash();

      ParameterMap params;
      params["hash"] = head_hash.HexString();
      RequestPtr request = NewGetRequest(session, "/chain/data/" + head_hash.HexString(), params);
      ResponsePtr response = NewOkResponse(session, head);

      EXPECT_CALL((MockBlockChain&)*chain, GetBlock(IsHash(head_hash)))
          .Times(::testing::AtLeast(1))
          .WillRepeatedly(::testing::Return(head));
      EXPECT_CALL((MockBlockChain&)*chain, HasBlock(IsHash(head_hash)))
          .Times(::testing::AtLeast(1))
          .WillRepeatedly(::testing::Return(true));
      EXPECT_CALL((MockHttpSession&)*session, Send(ResponseEquals(response)))
          .Times(::testing::AtLeast(1));
      controller.OnGetBlockChainBlock(session, request);
    }
  }
}