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

      RequestPtr request = NewGetRequest("/chain/head");
      ResponsePtr response = NewOkResponse(head);

      EXPECT_CALL((MockBlockChain&)*chain, GetHead())
          .Times(::testing::AtLeast(1))
          .WillRepeatedly(::testing::Return(head));
      EXPECT_CALL((MockHttpSession&)*session, Send(ResponseEquals(response)))
          .Times(::testing::AtLeast(1));
      controller.OnGetBlockChainHead(session, request);
    }

//TODO:
//
//    TEST_F(ChainControllerTest, TestGetBlockChain){
//      SessionPtr session = NewMockHttpSession();
//      ::testing::Mock::AllowLeak(session.get()); // not a leak, this is a smart ptr
//
//      BlockChainPtr chain = NewMockBlockChain();
//      ::testing::Mock::AllowLeak(chain.get()); // not a leak, this is a smart ptr
//
//      ChainController controller(chain);
//
//      BlockPtr head = Block::Genesis();
//
//      RequestPtr request = NewGetRequest("/chain");
//      ResponsePtr response = NewOkResponse(HashList{ head->hash() });
//
//      EXPECT_CALL((MockBlockChain&)*chain, GetHead())
//          .Times(::testing::AtLeast(1))
//          .WillRepeatedly(::testing::Return(head));
//      EXPECT_CALL((MockHttpSession&)*session, Send(ResponseEquals(response)))
//          .Times(::testing::AtLeast(1));
//      controller.OnGetBlockChain(session, request);
//    }

    // test for a successful response w/ valid inputs
    TEST_F(ChainControllerTest, TestGetBlockChainBlock1){
      SessionPtr session = NewMockHttpSession();
      ::testing::Mock::AllowLeak(session.get()); // not a leak, this is a shared ptr

      BlockChainPtr chain = NewMockBlockChain();
      ChainController controller(chain);

      BlockPtr head = Block::Genesis();
      Hash head_hash = head->hash();

      ParameterMap params;
      params["hash"] = head_hash.HexString();
      RequestPtr request = NewGetRequest("/chain/data/" + head_hash.HexString(), params);
      ResponsePtr response = NewOkResponse(head);

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

    // test for an unsuccessful response w/ invalid inputs
    TEST_F(ChainControllerTest, TestGetBlockChainBlock2){
      SessionPtr session = NewMockHttpSession();
      BlockChainPtr chain = NewMockBlockChain();
      ChainController controller(chain);

      Hash target = Hash::GenerateNonce(); // I apologize in advance if this happens to collide!
      ParameterMap params;
      params["hash"] = target.HexString();

      RequestPtr request = NewGetRequest("/chain/data/" + target.HexString(), params);
      ResponsePtr response = NewNoContentResponse(target);

      EXPECT_CALL((MockBlockChain&)*chain, HasBlock(IsHash(target)))
          .Times(::testing::AtLeast(1))
          .WillRepeatedly(::testing::Return(false));
      EXPECT_CALL((MockHttpSession&)*session, Send(ResponseEquals(response)))
          .Times(::testing::AtLeast(1));
      controller.OnGetBlockChainBlock(session, request);
    }
  }
}