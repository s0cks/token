#include "json.h"
#include "http/test_http_controller_chain.h"

namespace token{
  static inline std::string
  GetRequestPathForBlock(const Hash& hash){
    std::stringstream ss;
    ss << "/chain/data/";
    ss << hash;
    return ss.str();
  }

  static inline std::string
  GetResponseBodyForBlock(const BlockPtr& blk){
    Json::String body;
    Json::Writer writer(body);
    if(!writer.StartObject()){
      LOG(ERROR) << "cannot convert Block to Json";
      return "{}";
    }

    if(!Json::SetField(writer, "data", blk)){
      LOG(ERROR) << "cannot convert Block to Json";
      return "{}";
    }

    return std::string(body.GetString(), body.GetSize());
  }

  static inline std::string
  GetResponseBodyForHashList(const HashList& hashes){
    Json::String body;
    Json::Writer writer(body);
    if(!writer.StartArray()){
      LOG(ERROR) << "cannot convert HashList to json";
      return "{}";
    }

    if(!Json::SetField(writer, "data", hashes)){
      LOG(ERROR) << "cannot convert HashList to Json";
      return "{}";
    }

    return std::string(body.GetString(), body.GetSize());
  }

  TEST_F(ChainControllerTest, TestGetBlockChainHead){
    http::SessionPtr session = NewMockHttpSession();

    BlockPtr head = Block::Genesis();
    EXPECT_CALL((MockBlockChain&)*GetChain(), GetHead())
      .Times(::testing::AtLeast(1))
      .WillRepeatedly(::testing::Return(head));

    // Expected
    EXPECT_CALL((MockHttpSession&)*session, Send(::testing::AllOf(ResponseIsOk())))
      .Times(::testing::AtLeast(1));
    GetController()->OnGetBlockChainHead(session, nullptr);
  }

  TEST_F(ChainControllerTest, TestGetBlockChain){
    http::SessionPtr session = NewMockHttpSession();

    BlockPtr head = Block::Genesis();
    EXPECT_CALL((MockBlockChain&)*GetChain(), GetHead())
      .Times(::testing::AtLeast(1))
      .WillRepeatedly(::testing::Return(head));

    HashList expected;
    expected << head;
    EXPECT_CALL((MockHttpSession&)*session, Send(::testing::AllOf(ResponseIsOk(), ResponseBodyEqualsString(GetResponseBodyForHashList(expected)))))
        .Times(::testing::AtLeast(1));
    GetController()->OnGetBlockChain(session, nullptr);
  }

  TEST_F(ChainControllerTest, TestGetBlockChainBlock){
    http::SessionPtr session = NewMockHttpSession();

    BlockPtr blk = Block::Genesis();

    Hash hash = blk->GetHash();
    EXPECT_CALL((MockBlockChain&)*GetChain(), GetBlock(IsHash(hash)))
      .Times(::testing::AtLeast(1))
      .WillRepeatedly(::testing::Return(blk));
    EXPECT_CALL((MockBlockChain&)*GetChain(), HasBlock(IsHash(hash)))
      .Times(::testing::AtLeast(1))
      .WillRepeatedly(::testing::Return(true));
    
    http::RequestBuilder builder(session);
    builder.SetMethod(http::Method::kGet);
    builder.SetPath(GetRequestPathForBlock(hash));
    builder.SetPathParameter("hash", hash.HexString());

    EXPECT_CALL((MockHttpSession&)*session, Send(::testing::AllOf(ResponseIsOk(), ResponseBodyEqualsString(GetResponseBodyForBlock(blk)))))
      .Times(::testing::AtLeast(1));
    GetController()->OnGetBlockChainBlock(session, builder.Build());
  }
}