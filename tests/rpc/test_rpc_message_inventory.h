#ifndef TOKEN_TEST_RPC_MESSAGE_INVENTORY_H
#define TOKEN_TEST_RPC_MESSAGE_INVENTORY_H

#include "test_suite.h"
#include "rpc/messages/rpc_message_inventory.h"

namespace token{
  class InventoryItemTest : public ::testing::Test{
   protected:
    InventoryItemTest() = default;
   public:
    ~InventoryItemTest() override = default;
  };

  template<class M>
  class InventoryMessageTest : public MessageTest<M>{
   protected:
    InventoryMessageTest() = default;
   public:
    ~InventoryMessageTest() override = default;
  };

  class GetDataMessageTest : public InventoryMessageTest<rpc::GetDataMessage>{
   protected:
    Type type_;
    Hash hash_;

    GetDataMessageTest():
      InventoryMessageTest<rpc::GetDataMessage>(),
      type_(Type::kBlock),
      hash_(Hash::GenerateNonce()){}

    rpc::GetDataMessagePtr CreateMessage() const override{
      InventoryItems items = { InventoryItem(type_, hash_) };
      return rpc::GetDataMessage::NewInstance(items);
    }
   public:
    ~GetDataMessageTest() override = default;
  };
}

#endif//TOKEN_TEST_RPC_MESSAGE_INVENTORY_H