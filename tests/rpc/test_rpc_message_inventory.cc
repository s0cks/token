#include "rpc/test_rpc_message_inventory.h"

namespace token{
  TEST_F(InventoryItemTest, EqualsTest){
    Type item_type = Type::kBlock;
    Hash item_hash = Hash::GenerateNonce();
    InventoryItem item(item_type, item_hash);
    ASSERT_EQ(item, InventoryItem(item_type, item_hash)) << "items aren't equal";
  }

  TEST_F(InventoryItemTest, WriteTest){
    Hash item_hash = Hash::GenerateNonce();
    Type item_type = Type::kBlock;
    InventoryItem item1(item_type, item_hash);
    BufferPtr tmp = Buffer::NewInstance(item1.GetBufferSize());
    ASSERT_TRUE(item1.Write(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), item1.GetBufferSize());
    ASSERT_EQ(item1, InventoryItem(tmp));
  }

  TEST_F(GetDataMessageTest, EqualsTest){
    GetDataMessagePtr a = CreateMessage();
    DLOG(INFO) << "a: " << a->ToString();
    GetDataMessagePtr b = CreateMessage();
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(GetDataMessageTest, WriteMessageTest){
    GetDataMessagePtr a = CreateMessage();
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    GetDataMessagePtr b = GetDataMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }
}