#include "test_suite.h"
#include "message.h"

namespace Token{
  static inline MessagePtr
  NewVersionMessage(){
    return VersionMessage::NewInstance(UUID());
  }

  static inline MessagePtr
  NewVerackMessage(){
    BlockPtr head = Block::Genesis();
    return VerackMessage::NewInstance(ClientType::kNode, UUID(), NodeAddress(), Version(), head->GetHeader(), Hash::GenerateNonce());
  }

  static inline MessagePtr
  NewPrepareMessage(){
    int64_t height = 1;
    UUID uuid("f5c39f32-536b-11eb-a930-516c7b33ab9a");
    Hash hash = Hash::FromHexString("1191F9F9657BB3AB6D6E043D7B4017507D802795FE58EFA3043A066980C32C72");
    ProposalPtr proposal = std::make_shared<Proposal>(uuid, height, hash);
    return PrepareMessage::NewInstance(proposal);
  }

  static inline MessagePtr
  NewPromiseMessage(){
    int64_t height = 1;
    UUID uuid("f5c39f32-536b-11eb-a930-516c7b33ab9a");
    Hash hash = Hash::FromHexString("1191F9F9657BB3AB6D6E043D7B4017507D802795FE58EFA3043A066980C32C72");
    ProposalPtr proposal = std::make_shared<Proposal>(uuid, height, hash);
    return PromiseMessage::NewInstance(proposal);
  }

  static inline MessagePtr
  NewCommitMessage(){
    int64_t height = 1;
    UUID uuid("f5c39f32-536b-11eb-a930-516c7b33ab9a");
    Hash hash = Hash::FromHexString("1191F9F9657BB3AB6D6E043D7B4017507D802795FE58EFA3043A066980C32C72");
    ProposalPtr proposal = std::make_shared<Proposal>(uuid, height, hash);
    return CommitMessage::NewInstance(proposal);
  }

  static inline MessagePtr
  NewAcceptedMessage(){
    int64_t height = 1;
    UUID uuid("f5c39f32-536b-11eb-a930-516c7b33ab9a");
    Hash hash = Hash::FromHexString("1191F9F9657BB3AB6D6E043D7B4017507D802795FE58EFA3043A066980C32C72");
    ProposalPtr proposal = std::make_shared<Proposal>(uuid, height, hash);
    return AcceptedMessage::NewInstance(proposal);
  }

  static inline MessagePtr
  NewRejectedMessage(){
    int64_t height = 1;
    UUID uuid("f5c39f32-536b-11eb-a930-516c7b33ab9a");
    Hash hash = Hash::FromHexString("1191F9F9657BB3AB6D6E043D7B4017507D802795FE58EFA3043A066980C32C72");
    ProposalPtr proposal = std::make_shared<Proposal>(uuid, height, hash);
    return RejectedMessage::NewInstance(proposal);
  }

  static inline MessagePtr
  NewGetDataMessage(){
    std::vector<InventoryItem> items = {
      InventoryItem(InventoryItem::kBlock, Hash::GenerateNonce()),
    };
    return GetDataMessage::NewInstance(items);
  }

  static inline MessagePtr
  NewGetBlocksMessage(){
    return GetBlocksMessage::NewInstance(Hash::GenerateNonce(), Hash::GenerateNonce());
  }

  static inline MessagePtr
  NewBlockMessage(){
    return BlockMessage::NewInstance(Block::Genesis());
  }

  static inline MessagePtr
  NewTransactionMessage(){
    InputList inputs = {
      Input(Hash::GenerateNonce(), 0, "TestUser"),
    };
    OutputList outputs = {
      Output("TestUser", "TestToken"),
    };
    return TransactionMessage::NewInstance(Transaction::NewInstance(0, inputs, outputs));
  }

  static inline MessagePtr
  NewInventoryMessage(){
    return InventoryMessage::NewInstance(Hash::GenerateNonce(), InventoryItem::kBlock);
  }

  static inline MessagePtr
  NewNotFoundMessage(){
    return NotFoundMessage::NewInstance();
  }

  static inline MessagePtr
  NewGetPeersMessage(){
    return GetPeersMessage::NewInstance();
  }

  static inline MessagePtr
  NewPeerListMessage(){
    PeerList peers = {
      NodeAddress("localhost:8080"),
    };
    return PeerListMessage::NewInstance(peers);
  }

#define DEFINE_MESSAGE_SERIALIZATION_TEST_CASE(Name) \
  TEST(Test##Name##MessageSerialization, test_pos){  \
    MessagePtr a = New##Name##Message();             \
    BufferPtr tmp = Buffer::NewInstance(a->GetBufferSize()); \
    ASSERT_TRUE(a->Write(tmp));                      \
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetBufferSize()); \
    ASSERT_EQ(tmp->GetInt(), static_cast<int32_t>(Message::k##Name##MessageType)); \
    ASSERT_EQ(tmp->GetLong(), (a->GetBufferSize() - Message::kHeaderSize));        \
    MessagePtr b = Name##Message::NewInstance(tmp);  \
    ASSERT_TRUE(a->Equals(b));                       \
  }
  FOR_EACH_MESSAGE_TYPE(DEFINE_MESSAGE_SERIALIZATION_TEST_CASE);
#undef DEFINE_MESSAGE_SERIALIZATION_TEST_CASE
}