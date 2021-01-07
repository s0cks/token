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
    ProposalPtr proposal = std::make_shared<Proposal>(UUID(), 0, Hash::GenerateNonce());
    return PrepareMessage::NewInstance(proposal);
  }

  static inline MessagePtr
  NewPromiseMessage(){
    ProposalPtr proposal = std::make_shared<Proposal>(UUID(), 0, Hash::GenerateNonce());
    return PromiseMessage::NewInstance(proposal);
  }

  static inline MessagePtr
  NewCommitMessage(){
    ProposalPtr proposal = std::make_shared<Proposal>(UUID(), 0, Hash::GenerateNonce());
    return CommitMessage::NewInstance(proposal);
  }

  static inline MessagePtr
  NewAcceptedMessage(){
    ProposalPtr proposal = std::make_shared<Proposal>(UUID(), 0, Hash::GenerateNonce());
    return AcceptedMessage::NewInstance(proposal);
  }

  static inline MessagePtr
  NewRejectedMessage(){
    ProposalPtr proposal = std::make_shared<Proposal>(UUID(), 0, Hash::GenerateNonce());
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
    ASSERT_EQ(tmp->GetInt(), static_cast<int32_t>(Message::k##Name##MessageType)); \
    ASSERT_EQ(tmp->GetLong(), (a->GetBufferSize() - Message::kHeaderSize));        \
    MessagePtr b = Name##Message::NewInstance(tmp);  \
    ASSERT_TRUE(a->Equals(b));                       \
  }
  FOR_EACH_MESSAGE_TYPE(DEFINE_MESSAGE_SERIALIZATION_TEST_CASE);
#undef DEFINE_MESSAGE_SERIALIZATION_TEST_CASE
}