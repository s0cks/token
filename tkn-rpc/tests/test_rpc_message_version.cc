#include "test_rpc_message_version.h"

namespace token{
  namespace rpc{
    TEST_F(VersionMessageTest, TestSerialization){
      rpc::VersionMessage a(Clock::now(), ClientType::kNode, Version::CurrentVersion(), Hash::GenerateNonce(), UUID());
      auto data = a.ToBuffer();
      rpc::MessageParser parser(data);
      ASSERT_TRUE(parser.HasNext());
      auto pnext = parser.Next();
      ASSERT_EQ(pnext->type(), Type::kVersionMessage);
      auto next = std::static_pointer_cast<rpc::VersionMessage>(pnext);
      ASSERT_EQ(next->timestamp(), a.timestamp());
      ASSERT_EQ(next->client_type(), a.client_type());
      ASSERT_EQ(next->version(), a.version());
      ASSERT_EQ(next->nonce(), a.nonce());
      ASSERT_EQ(next->node_id(), a.node_id());
    }
  }
}