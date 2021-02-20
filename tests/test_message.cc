#include "test_suite.h"
#include "rpc/rpc_message.h"

namespace token{
  static inline RpcMessagePtr
  NewVersionMessage(){
    return VersionMessage::NewInstance(UUID());
  }

  static inline RpcMessagePtr
  NewVerackMessage(){
    BlockPtr head = Block::Genesis();
    Version version = Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    return VerackMessage::NewInstance(ClientType::kNode, UUID(), NodeAddress(), version, head->GetHeader(), Hash::GenerateNonce());
  }

  static inline ProposalPtr
  NewProposal(){
    BlockPtr blk = Block::Genesis();
    UUID uuid("f5c39f32-536b-11eb-a930-516c7b33ab9a");
    return std::make_shared<Proposal>(uuid, blk->GetHeader());
  }

  static inline RpcMessagePtr
  NewPrepareMessage(){
    return PrepareMessage::NewInstance(NewProposal());
  }

  static inline RpcMessagePtr
  NewPromiseMessage(){
    return PromiseMessage::NewInstance(NewProposal());
  }

  static inline RpcMessagePtr
  NewCommitMessage(){
    return CommitMessage::NewInstance(NewProposal());
  }

  static inline RpcMessagePtr
  NewAcceptedMessage(){
    return AcceptedMessage::NewInstance(NewProposal());
  }

  static inline RpcMessagePtr
  NewRejectedMessage(){
    return RejectedMessage::NewInstance(NewProposal());
  }

  static inline RpcMessagePtr
  NewGetDataMessage(){
    std::vector<InventoryItem> items = {
      InventoryItem(InventoryItem::kBlock, Hash::GenerateNonce()),
    };
    return GetDataMessage::NewInstance(items);
  }

  static inline RpcMessagePtr
  NewGetBlocksMessage(){
    return GetBlocksMessage::NewInstance(Hash::GenerateNonce(), Hash::GenerateNonce());
  }

  static inline RpcMessagePtr
  NewBlockMessage(){
    return BlockMessage::NewInstance(Block::Genesis());
  }

  static inline RpcMessagePtr
  NewTransactionMessage(){
    InputList inputs = {
      Input(Hash::GenerateNonce(), 0, "TestUser"),
    };
    OutputList outputs = {
      Output("TestUser", "TestToken"),
    };
    return TransactionMessage::NewInstance(Transaction::NewInstance(inputs, outputs));
  }

  static inline RpcMessagePtr
  NewInventoryMessage(){
    return InventoryMessage::NewInstance(Hash::GenerateNonce(), InventoryItem::kBlock);
  }

  static inline RpcMessagePtr
  NewNotFoundMessage(){
    return NotFoundMessage::NewInstance();
  }

#define DEFINE_MESSAGE_SERIALIZATION_TEST_CASE(Name) \
  TEST(Test##Name##MessageSerialization, test_pos){  \
    RpcMessagePtr a = New##Name##Message();             \
    BufferPtr tmp = Buffer::NewInstance(a->GetBufferSize()); \
    ASSERT_TRUE(a->Write(tmp));                      \
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetBufferSize());   \
    ASSERT_EQ(tmp->GetObjectTag(), ObjectTag(Type::k##Name##Message, a->GetBufferSize())); \
    RpcMessagePtr b = Name##Message::NewInstance(tmp);  \
    ASSERT_TRUE(a->Equals(b));                       \
  }
  FOR_EACH_MESSAGE_TYPE(DEFINE_MESSAGE_SERIALIZATION_TEST_CASE);
#undef DEFINE_MESSAGE_SERIALIZATION_TEST_CASE
}