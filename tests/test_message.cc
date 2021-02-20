#include "test_message.h"

namespace token{
  VersionMessagePtr VersionMessageTest::CreateMessage() const{
    ClientType type = ClientType::kNode;
    UUID node_id;
    BlockPtr head = Block::Genesis();
    Version version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    Hash nonce = Hash::GenerateNonce();
    Timestamp timestamp = Clock::now();
    return VersionMessage::NewInstance(
      type,
      node_id,
      head->GetHeader(),
      version,
      nonce,
      timestamp
    );
  }

  VerackMessagePtr VerackMessageTest::CreateMessage() const{
    ClientType type = ClientType::kNode;
    UUID node_id;
    BlockPtr head = Block::Genesis();
    Version version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    NodeAddress callback("127.0.0.1:8080");
    Hash nonce = Hash::GenerateNonce();
    Timestamp timestamp = Clock::now();
    return VerackMessage::NewInstance(
      type,
      node_id,
      callback,
      version,
      head->GetHeader(),
      nonce,
      timestamp
    );
  }

  static inline ProposalPtr
  CreateProposal(){
    BlockPtr blk = Block::Genesis();
    UUID uuid("f5c39f32-536b-11eb-a930-516c7b33ab9a");
    return std::make_shared<Proposal>(uuid, blk->GetHeader());
  }

  PrepareMessagePtr PrepareMessageTest::CreateMessage() const{
    return PrepareMessage::NewInstance(CreateProposal());
  }

  PromiseMessagePtr PromiseMessageTest::CreateMessage() const{
    return PromiseMessage::NewInstance(CreateProposal());
  }

  CommitMessagePtr CommitMessageTest::CreateMessage() const{
    return CommitMessage::NewInstance(CreateProposal());
  }

  AcceptedMessagePtr AcceptedMessageTest::CreateMessage() const{
    return AcceptedMessage::NewInstance(CreateProposal());
  }

  RejectedMessagePtr RejectedMessageTest::CreateMessage() const{
    return RejectedMessage::NewInstance(CreateProposal());
  }

  BlockMessagePtr BlockMessageTest::CreateMessage() const{
    return BlockMessage::NewInstance(Block::Genesis());
  }

  TransactionMessagePtr TransactionMessageTest::CreateMessage() const{
    InputList inputs = {};
    OutputList outputs = {};
    Timestamp timestamp = FromUnixTimestamp(0);
    TransactionPtr tx = Transaction::NewInstance(inputs, outputs, timestamp);
    return TransactionMessage::NewInstance(tx);
  }

  GetDataMessagePtr GetDataMessageTest::CreateMessage() const{
    std::vector<InventoryItem> items = {
      InventoryItem(InventoryItem::kBlock, Hash::GenerateNonce()),
    };
    return GetDataMessage::NewInstance(items);
  }

  GetBlocksMessagePtr GetBlocksMessageTest::CreateMessage() const{
    return GetBlocksMessage::NewInstance(Hash::GenerateNonce(), Hash::GenerateNonce());
  }

  InventoryMessagePtr InventoryMessageTest::CreateMessage() const{
    std::vector<InventoryItem> items = {
      InventoryItem(InventoryItem::kBlock, Hash::GenerateNonce()),
    };
    return InventoryMessage::NewInstance(items);
  }

  NotFoundMessagePtr NotFoundMessageTest::CreateMessage() const{
    return NotFoundMessage::NewInstance();
  }

#define DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Name) \
  TEST_F(Name##MessageTest, test_serialization){    \
    Name##MessagePtr a = CreateMessage();           \
    int64_t size = a->GetBufferSize();              \
    BufferPtr tmp = Buffer::NewInstance(size);      \
    ASSERT_TRUE(a->Write(tmp));                     \
    ASSERT_EQ(tmp->GetWrittenBytes(), size);        \
    ASSERT_EQ(tmp->GetObjectTag(), ObjectTag(Type::k##Name##Message, size)); \
    Name##MessagePtr b = Name##Message::NewInstance(tmp);                    \
    ASSERT_TRUE(a->Equals(b));                      \
  }
  FOR_EACH_MESSAGE_TYPE(DEFINE_MESSAGE_SERIALIZATION_TESTCASE)
#undef DEFINE_MESSAGE_SERIALIZATION_TESTCASE
}