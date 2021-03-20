#include "rpc/test_rpc_server_session.h"
#include "mock/mock_object_pool.h"
#include "transaction.h"

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

    MockBlockChainPtr chain = std::make_shared<MockBlockChain>();
    MockObjectPoolPtr pool = std::make_shared<MockObjectPool>();

    EXPECT_CALL(*chain, GetHead())
      .Times(1)
      .WillRepeatedly(testing::Return(head));

    MockServerSession session(pool, chain);
    EXPECT_CALL(session, Send(IsVersionMessage(timestamp, client_type, version, nonce, node_id, head->GetHeader())))
      .Times(testing::AtLeast(1));
    session.OnVersionMessage(message);
  }

  TEST_F(ServerSessionTest, TestOnVerackMessage){

  }

  MATCHER_P(IsMessageListOf, b, "Messages are not equal"){
    RpcMessageList& a = (RpcMessageList&)arg;
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

  TEST_F(ServerSessionTest, TestOnGetDataMessage){
    Hash blk1_hash = Hash::GenerateNonce(); // found in pool
    BlockPtr blk1 = Block::Genesis();//TODO: convert to random assignment

    Hash blk2_hash = Hash::GenerateNonce(); // found in chain
    BlockPtr blk2 = Block::Genesis();//TODO: convert to random assignment

    Hash blk3_hash = Hash::GenerateNonce(); // not found

    MockBlockChainPtr chain = MockBlockChain::NewInstance();
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

    MockObjectPoolPtr pool = MockObjectPool::NewInstance();
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

    RpcMessageList messages;
    messages << BlockMessage::NewInstance(blk1);
    messages << BlockMessage::NewInstance(blk2);
    messages << NotFoundMessage::NewInstance("cannot find");

    MockServerSession session(pool, chain);
    EXPECT_CALL(session, SendMessages(IsMessageListOf(messages)))
      .Times(testing::AtLeast(1));

    InventoryItems items;
    items << InventoryItem(Type::kBlock, blk1_hash);
    items << InventoryItem(Type::kBlock, blk2_hash);
    items << InventoryItem(Type::kBlock, blk3_hash);
    session.OnGetDataMessage(GetDataMessage::NewInstance(items));
  }

  TEST_F(ServerSessionTest, TestOnGetBocksMessage){

  }

  /**
   * Test Case
   *
   * Asserts that if a received Block isn't already found in the ObjectPool, then
   * it will add it to the ObjectPool and relay it to it's peers.
   */
  TEST_F(ServerSessionTest, TestOnBlockMessage1){
    BlockPtr blk = Block::Genesis();
    Hash hash = blk->GetHash();

    MockBlockChainPtr chain = std::make_shared<MockBlockChain>();

    MockObjectPoolPtr pool = std::make_shared<MockObjectPool>();
    EXPECT_CALL(*pool, HasBlock(IsHash(hash)))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*pool, PutBlock(IsHash(hash), IsBlock(hash)))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(true));

    MockServerSession session(pool, chain);
    session.OnBlockMessage(BlockMessage::NewInstance(blk));
  }

  /**
   * Test Case
   *
   * Asserts that if a received Block is already found in the ObjectPool, then
   * it doesn't relay the Block to it's peers.
   */
  TEST_F(ServerSessionTest, TestOnBlockMessage2){
    BlockPtr blk = Block::Genesis();
    Hash hash = blk->GetHash();

    MockBlockChainPtr chain = std::make_shared<MockBlockChain>();

    MockObjectPoolPtr pool = std::make_shared<MockObjectPool>();
    EXPECT_CALL(*pool, HasBlock(IsHash(hash)))
        .Times(testing::AtLeast(1))
        .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*pool, PutBlock(IsHash(hash), IsBlock(hash)))
        .Times(testing::AtMost(0));

    MockServerSession session(pool, chain);
    session.OnBlockMessage(BlockMessage::NewInstance(blk));
  }

  /**
   * Test Case
   *
   * Asserts that if a received Transaction isn't already found in the ObjectPool, then
   * it will add it to the ObjectPool and relay it to it's peers.
   */
  TEST_F(ServerSessionTest, TestOnTransactionMessage1){
    TransactionPtr tx = CreateRandomTransaction(1, 1);
    Hash hash = tx->GetHash();

    MockObjectPoolPtr pool = std::make_shared<MockObjectPool>();
    EXPECT_CALL(*pool, HasTransaction(IsHash(hash)))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*pool, PutTransaction(IsHash(hash), IsTransaction(tx)))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(true));

    MockBlockChainPtr chain = std::make_shared<MockBlockChain>();

    MockServerSession session(pool, chain);
    session.OnTransactionMessage(TransactionMessage::NewInstance(tx));
  }

  /**
   * Test Case
   *
   * Asserts that if a received Transaction is already found in the ObjectPool, then
   * it doesn't relay the Transaction to it's peers.
   */
  TEST_F(ServerSessionTest, TestOnTransactionMessage2){
    TransactionPtr tx = CreateRandomTransaction(1, 1);
    Hash hash = tx->GetHash();

    MockObjectPoolPtr pool = std::make_shared<MockObjectPool>();
    EXPECT_CALL(*pool, HasTransaction(IsHash(hash)))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*pool, PutTransaction(IsHash(hash), AnyTransaction()))
      .Times(testing::AtMost(0));

    MockBlockChainPtr chain = std::make_shared<MockBlockChain>();

    MockServerSession session(pool, chain);
    session.OnTransactionMessage(TransactionMessage::NewInstance(tx));
  }

  /**
   * Test Case
   *
   * Asserts that when an inventory list is received it will:
   *    - fetch objects that are not found in the chain or pool
   *    - not fetch objects that are found in the chain or pool
   */
  TEST_F(ServerSessionTest, TestOnInventoryListMessage1){
    Hash blk1_hash = Hash::GenerateNonce(); // not found
    Hash blk2_hash = Hash::GenerateNonce(); // found in the pool
    Hash blk3_hash = Hash::GenerateNonce(); // found in the block chain
    InventoryItems items;
    items.insert(InventoryItem(Type::kBlock, blk1_hash));
    items.insert(InventoryItem(Type::kBlock, blk2_hash));
    items.insert(InventoryItem(Type::kBlock, blk3_hash));

    MockBlockChainPtr chain = std::make_shared<MockBlockChain>();
    EXPECT_CALL(*chain, HasBlock(IsHash(blk1_hash)))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*chain, HasBlock(IsHash(blk2_hash)))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*chain, HasBlock(IsHash(blk3_hash)))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(true));

    MockObjectPoolPtr pool = std::make_shared<MockObjectPool>();
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

    MockServerSession session(pool, chain);
    EXPECT_CALL(session, Send(IsGetDataMessage(needed)))
      .Times(testing::AtLeast(1));
    session.OnInventoryListMessage(InventoryListMessage::NewInstance(items));
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
