#include "rpc/test_rpc_server_message_handler.h"
#include "mock/mock_object_pool.h"

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
        auto bval = (*biter);
        if(!aval->Equals(bval)){
          LOG(WARNING) << "a (" << aval->ToString() << ") != b (" << bval->ToString() << ")";
          return false;
        }
        aiter++;
        biter++;
      }
      return true;
    }

    MATCHER_P(VersionMessageHasNonce, nonce, "The VersionMessage doesn't have the expected nonce"){
      auto msg = std::static_pointer_cast<VersionMessage>(arg);
      return msg->GetNonce() == nonce;
    }

    TEST_F(ServerMessageHandlerTest, TestOnVersionMessageAccepted){
      MockBlockChainPtr chain = MockBlockChain::NewInstance();
      MockObjectPoolPtr pool = MockObjectPool::NewInstance();
      MockServerSessionPtr session = MockServerSession::NewInstance(chain, pool);
      rpc::MessageHandler& handler = session->GetMessageHandler();

      Hash nonce = Hash::GenerateNonce();
      VersionMessagePtr message = VersionMessage::NewInstance(
         Clock::now(),
         ClientType::kNode,
         Version::CurrentVersion(),
         nonce,
         UUID(),
         Block::Genesis()
      );

      EXPECT_CALL(*chain, GetHead())
        .WillRepeatedly(testing::Return(Block::Genesis()));
      EXPECT_CALL(*session, Send(::testing::AllOf(IsVersionMessage(), VersionMessageHasNonce(nonce))))
        .Times(testing::Exactly(1));

      handler.OnVersionMessage(message);
    }

    TEST_F(ServerMessageHandlerTest, TestOnVerackMessage){
      MockBlockChainPtr chain = MockBlockChain::NewInstance();
      MockObjectPoolPtr pool = MockObjectPool::NewInstance();
      MockServerSessionPtr session = MockServerSession::NewInstance(chain, pool);
      rpc::MessageHandler& handler = session->GetMessageHandler();

      BlockPtr genesis = Block::Genesis();

      rpc::VerackMessagePtr msg = VerackMessage::NewInstance(
         ClientType::kNode,
         UUID(),
         NodeAddress(),
         Version::CurrentVersion(),
         genesis->GetHeader(),
         Hash::GenerateNonce(),
         Clock::now()
      );


      EXPECT_CALL(*chain, GetHead)
        .Times(testing::AtLeast(1))
        .WillRepeatedly(testing::Return(genesis));

      handler.OnVerackMessage(msg);
    }

    TEST_F(ServerMessageHandlerTest, TestOnGetBocksMessage){
    }

    TEST_F(ServerMessageHandlerTest, TestOnGetDataMessage){
      MockObjectPoolPtr pool = MockObjectPool::NewInstance();
      MockBlockChainPtr chain = MockBlockChain::NewInstance();
      MockServerSession session(pool, chain);
      ServerMessageHandler handler((ServerSession*) &session);
      Hash blk1_hash = Hash::GenerateNonce(); // found in pool
      BlockPtr blk1 = Block::Genesis();//TODO: convert to random assignment

      Hash blk2_hash = Hash::GenerateNonce(); // found in chain
      BlockPtr blk2 = Block::Genesis();//TODO: convert to random assignment

      Hash blk3_hash = Hash::GenerateNonce(); // not found

      EXPECT_CALL(*chain, HasBlock(IsHash(blk1_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(true));
      EXPECT_CALL(*chain, GetBlock(IsHash(blk1_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(blk1));
      EXPECT_CALL(*chain, HasBlock(IsHash(blk2_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(false));
      EXPECT_CALL(*chain, HasBlock(IsHash(blk3_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(false));
      EXPECT_CALL(*pool, HasBlock(IsHash(blk1_hash)))
         .Times(testing::AtMost(0))
         .WillRepeatedly(testing::Return(false));
      EXPECT_CALL(*pool, HasBlock(IsHash(blk2_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(true));
      EXPECT_CALL(*pool, GetBlock(IsHash(blk2_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(blk2));
      EXPECT_CALL(*pool, HasBlock(IsHash(blk3_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(false));
      rpc::MessageList messages;
      messages << BlockMessage::NewInstance(blk1);
      messages << BlockMessage::NewInstance(blk2);
      messages << NotFoundMessage::NewInstance(InventoryItem(Type::kBlock, blk1_hash), "cannot find");
      EXPECT_CALL(session, SendMessages(IsMessageListOf(messages)))
         .Times(testing::AtLeast(1));
      InventoryItems items;
      items << InventoryItem(Type::kBlock, blk1_hash);
      items << InventoryItem(Type::kBlock, blk2_hash);
      items << InventoryItem(Type::kBlock, blk3_hash);
      handler.OnGetDataMessage(GetDataMessage::NewInstance(items));
    }

    TEST_F(ServerMessageHandlerTest, TestOnBlockMessage1){
      MockObjectPoolPtr pool = MockObjectPool::NewInstance();
      MockBlockChainPtr chain = MockBlockChain::NewInstance();
      MockServerSession session(pool, chain);
      ServerMessageHandler handler((ServerSession*) &session);
      BlockPtr blk = Block::Genesis();
      Hash hash = blk->GetHash();
      EXPECT_CALL(*pool, HasBlock(IsHash(hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(false));
      EXPECT_CALL(*pool, PutBlock(IsHash(hash), IsBlock(hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(true));
      handler.OnBlockMessage(BlockMessage::NewInstance(blk));
    }

    TEST_F(ServerMessageHandlerTest, TestOnBlockMessage2){
      MockObjectPoolPtr pool = MockObjectPool::NewInstance();
      MockBlockChainPtr chain = MockBlockChain::NewInstance();
      MockServerSession session(pool, chain);
      ServerMessageHandler handler((ServerSession*) &session);
      BlockPtr blk = Block::Genesis();
      Hash hash = blk->GetHash();
      EXPECT_CALL(*pool, HasBlock(IsHash(hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(true));
      EXPECT_CALL(*pool, PutBlock(IsHash(hash), IsBlock(hash)))
         .Times(testing::AtMost(0));
      handler.OnBlockMessage(BlockMessage::NewInstance(blk));
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

    MATCHER_P(IsGetDataMessage, items, "Not an equal inventory"){
      if(arg->GetType() != Type::kGetDataMessage)
        return false;
      GetDataMessagePtr msg = std::static_pointer_cast<GetDataMessage>(arg);
      InventoryItems& a = msg->items();
      const InventoryItems& b = items;
      if(a.size() != b.size())
        return false;
      return a == b;
    }

    TEST_F(ServerMessageHandlerTest, TestOnTransactionMessage1){
      MockObjectPoolPtr pool = MockObjectPool::NewInstance();
      MockBlockChainPtr chain = MockBlockChain::NewInstance();
      MockServerSession session(pool, chain);
      ServerMessageHandler handler((ServerSession*) &session);
      TransactionPtr tx = CreateRandomTransaction(1, 1);
      Hash hash = tx->GetHash();
      EXPECT_CALL(*pool, HasTransaction(IsHash(hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(false));
      EXPECT_CALL(*pool, PutTransaction(IsHash(hash), IsTransaction(tx)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(true));
      handler.OnTransactionMessage(TransactionMessage::NewInstance(tx));
    }

    TEST_F(ServerMessageHandlerTest, TestOnTransactionMessage2){
      MockObjectPoolPtr pool = MockObjectPool::NewInstance();
      MockBlockChainPtr chain = MockBlockChain::NewInstance();
      MockServerSession session(pool, chain);
      ServerMessageHandler handler((ServerSession*) &session);
      TransactionPtr tx = CreateRandomTransaction(1, 1);
      Hash hash = tx->GetHash();
      EXPECT_CALL(*pool, HasTransaction(IsHash(hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(true));
      EXPECT_CALL(*pool, PutTransaction(IsHash(hash), AnyTransaction()))
         .Times(testing::AtMost(0));
      handler.OnTransactionMessage(TransactionMessage::NewInstance(tx));
    }

    TEST_F(ServerMessageHandlerTest, TestOnInventoryListMessage1){
      MockObjectPoolPtr pool = MockObjectPool::NewInstance();
      MockBlockChainPtr chain = MockBlockChain::NewInstance();
      MockServerSession session(pool, chain);
      ServerMessageHandler handler((ServerSession*) &session);
      Hash blk1_hash = Hash::GenerateNonce(); // not found
      Hash blk2_hash = Hash::GenerateNonce(); // found in the pool
      Hash blk3_hash = Hash::GenerateNonce(); // found in the block chain

      InventoryItems items;
      items << InventoryItem(Type::kBlock, blk1_hash);
      items << InventoryItem(Type::kBlock, blk2_hash);
      items << InventoryItem(Type::kBlock, blk3_hash);
      EXPECT_CALL(*chain, HasBlock(IsHash(blk1_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(false));
      EXPECT_CALL(*chain, HasBlock(IsHash(blk2_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(false));
      EXPECT_CALL(*chain, HasBlock(IsHash(blk3_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(true));
      EXPECT_CALL(*pool, HasBlock(IsHash(blk1_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(false));
      EXPECT_CALL(*pool, HasBlock(IsHash(blk2_hash)))
         .Times(testing::AtLeast(1))
         .WillRepeatedly(testing::Return(true));
      EXPECT_CALL(*pool, HasBlock(IsHash(blk3_hash)))
         .Times(testing::AtMost(0));

      // since blk1_hash is not found, it should be requested
      InventoryItems needed;
      needed << InventoryItem(Type::kBlock, blk1_hash);
      EXPECT_CALL(session, Send(IsGetDataMessage(needed)))
         .Times(testing::AtLeast(1));
      handler.OnInventoryListMessage(InventoryListMessage::NewInstance(items));
    }
  }
}