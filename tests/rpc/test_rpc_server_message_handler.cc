#include "rpc/test_rpc_server_message_handler.h"
#include "mock_object_pool.h"

namespace token{
  namespace rpc{
    MATCHER_P(IsMessageListOf, b, "Messages are not equal"){
      auto a = (rpc::MessageList&) arg;
      if(a.size() != b.size()){
        LOG(WARNING) << "not equal sizes";
        return false;
      }
      auto aiter = a.begin();
      auto biter = b.begin();
      while(aiter != a.end() || biter != b.end()){
        auto aval = (*aiter);
        auto bval = (*biter);/*
        if(!aval->Equals(bval)){
          LOG(WARNING) << "a (" << aval->ToString() << ") != b (" << bval->ToString() << ")";
          return false;
        }*/
        aiter++;
        biter++;
      }
      return true;
    }

    MATCHER_P(VersionMessageHasNonce, nonce, "The VersionMessage doesn't have the expected nonce"){
      auto msg = std::static_pointer_cast<VersionMessage>(arg);
      return msg->nonce() == nonce;
    }

    TEST_F(ServerMessageHandlerTest, TestOnVersionMessageAccepted){
      BlockChainPtr chain = NewMockBlockChain();
      ObjectPoolPtr pool = NewMockObjectPool();

      MockServerSessionPtr session = NewMockServerSession(chain, pool);
      rpc::MessageHandler& handler = session->GetMessageHandler();

      Hash nonce = Hash::GenerateNonce();
      VersionMessagePtr message = VersionMessage::NewInstance(
         Clock::now(),
         ClientType::kNode,
         Version::CurrentVersion(),
         nonce,
         UUID(),
         Block::Genesis()->GetHeader()
      );

      EXPECT_CALL((MockBlockChain&)*chain, GetHead())
        .WillRepeatedly(testing::Return(Block::Genesis()));
      EXPECT_CALL(*session, Send(::testing::AllOf(IsVersionMessage(), VersionMessageHasNonce(nonce))))
        .Times(testing::Exactly(1));

      handler.OnVersionMessage(message);
    }

    TEST_F(ServerMessageHandlerTest, TestOnVerackMessage){
      BlockChainPtr chain = NewMockBlockChain();
      ObjectPoolPtr pool = NewMockObjectPool();

      MockServerSessionPtr session = NewMockServerSession(chain, pool);
      rpc::MessageHandler& handler = session->GetMessageHandler();

      BlockPtr genesis = Block::Genesis();

      rpc::VerackMessagePtr msg = VerackMessage::NewInstance(
          Clock::now(),
         ClientType::kNode,
         Version::CurrentVersion(),
         Hash::GenerateNonce(),
         UUID(),
         genesis->GetHeader(),
         NodeAddress()
      );


      EXPECT_CALL((MockBlockChain&)*chain, GetHead)
        .Times(testing::AtLeast(1))
        .WillRepeatedly(testing::Return(genesis));

      handler.OnVerackMessage(msg);
    }

    TEST_F(ServerMessageHandlerTest, TestOnPrepareMessage){
    }

    TEST_F(ServerMessageHandlerTest, TestOnPromiseMessage){
    }

    TEST_F(ServerMessageHandlerTest, TestOnCommitMessage){
    }

    TEST_F(ServerMessageHandlerTest, TestOnAcceptedMessage){
    }

    TEST_F(ServerMessageHandlerTest, TestOnRejectedMessage){
    }
  }
}