#include "rpc/test_rpc_server_session.h"

namespace token{
#define DECLARE_RPC_MESSAGE_MATCHER(Name) \
  MATCHER(MessageIs##Name, ""){           \
    LOG(INFO) << "response is: " << arg->ToString(); \
    return arg->Is##Name##Message();      \
  }
  FOR_EACH_MESSAGE_TYPE(DECLARE_RPC_MESSAGE_MATCHER)
#undef DECLARE_RPC_MESSAGE_MATCHER

  MATCHER_P6(IsVersionMessage, timestamp, client_type, version, nonce, node_id, head, ""){
    VersionMessagePtr message = std::static_pointer_cast<VersionMessage>(arg);
    return message->GetClientType() == client_type
        && message->GetVersion() == version
        && message->GetNonce() == nonce
        && message->GetHead() == head;
  }

  TEST_F(ServerSessionTest, TestOnVersionMessage){
    Timestamp timestamp = Clock::now();
    ClientType client_type = ClientType::kNode;
    Version version = Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    Hash nonce = Hash::GenerateNonce();
    UUID node_id = UUID();
    BlockPtr head = Block::Genesis();

    VersionMessagePtr message = VersionMessage::NewInstance(
      timestamp,
      client_type,
      version,
      nonce,
      node_id,
      head->GetHeader()
    );

    std::shared_ptr<MockBlockChain> chain = std::make_shared<MockBlockChain>();
    EXPECT_CALL(*chain, GetHead())
      .Times(1)
      .WillRepeatedly(testing::Return(head));

    MockServerSession session(chain);
    EXPECT_CALL(session, Send(IsVersionMessage(timestamp, client_type, version, nonce, node_id, head->GetHeader())))
      .Times(testing::AtLeast(1));
    session.OnVersionMessage(message);
  }

  TEST_F(ServerSessionTest, TestOnVerackMessage){

  }

  TEST_F(ServerSessionTest, TestOnGetDataMessage){

  }

  TEST_F(ServerSessionTest, TestOnGetBocksMessage){

  }

  TEST_F(ServerSessionTest, TestOnBlockMessage){

  }

  TEST_F(ServerSessionTest, TestOnTransactionMessage){

  }

  TEST_F(ServerSessionTest, TestOnInventoryListMessage){

  }

  TEST_F(ServerSessionTest, TestOnPrepareMessage){

  }

  TEST_F(ServerSessionTest, TestOnPromiseMessage){

  }

  TEST_F(ServerSessionTest, TestOnCommitMessage){

  }

  TEST_F(ServerSessionTest, TestOnAcceptedMessage){

  }

  TEST_F(ServerSessionTest, TestOnRejectedMessage){

  }
}
