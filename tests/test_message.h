#ifndef TOKEN_TEST_MESSAGE_H
#define TOKEN_TEST_MESSAGE_H

#include "test_suite.h"
#include "rpc/rpc_message.h"

namespace token{
  template<class M>
  class MessageTest : public ::testing::Test{
   protected:
    typedef std::shared_ptr<M> MessageTypePtr;

    MessageTest() = default;
    virtual MessageTypePtr CreateMessage() const = 0;
   public:
    virtual ~MessageTest() = default;
  };

  class VersionMessageTest : public MessageTest<VersionMessage>{
   protected:
    VersionMessageTest() = default;

    virtual VersionMessagePtr CreateMessage() const{
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
  };

  class VerackMessageTest : public MessageTest<VerackMessage>{
   protected:
    VerackMessageTest():
      MessageTest<VerackMessage>(){}

    VerackMessagePtr CreateMessage() const{
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
   public:
    ~VerackMessageTest() = default;
  };

  template<class T, class M>
  class ObjectMessageTest : public MessageTest<M>{
   protected:
    typedef std::shared_ptr<T> ValuePtr;

    ObjectMessageTest():
      MessageTest<M>(){}

    virtual ValuePtr CreateValue() const = 0;

    std::shared_ptr<M> CreateMessage() const{
      return M::NewInstance(CreateValue());
    }
   public:
    ~ObjectMessageTest() = default;
  };

  class BlockMessageTest : public ObjectMessageTest<Block, BlockMessage>{
   protected:
    BlockMessageTest():
      ObjectMessageTest<Block, BlockMessage>(){}

    BlockPtr CreateValue() const{
      return Block::Genesis();
    }
   public:
    ~BlockMessageTest() = default;
  };

  class TransactionMessageTest : public ObjectMessageTest<Transaction, TransactionMessage>{
   protected:
    TransactionMessageTest():
      ObjectMessageTest<Transaction, TransactionMessage>(){}

    TransactionPtr CreateValue() const{
      InputList inputs = {};
      OutputList outputs = {};
      Timestamp timestamp = FromUnixTimestamp(0);
      return Transaction::NewInstance(inputs, outputs, timestamp);
    }
   public:
    ~TransactionMessageTest() = default;
  };

  template<class M>
  class InventoryMessageTest : public MessageTest<M>{
   protected:
    InventoryMessageTest() = default;
   public:
    ~InventoryMessageTest() = default;
  };

  class GetDataMessageTest : public InventoryMessageTest<GetDataMessage>{
   protected:
    GetDataMessageTest():
      InventoryMessageTest<GetDataMessage>(){}

    GetDataMessagePtr CreateMessage() const{
      InventoryItems items = { InventoryItem(Type::kBlock, Hash::GenerateNonce()) };
      return GetDataMessage::NewInstance(items);
    }
   public:
    ~GetDataMessageTest() = default;
  };

 class NotFoundMessageTest : public MessageTest<NotFoundMessage>{
  protected:
    NotFoundMessageTest():
      MessageTest<NotFoundMessage>(){}

    NotFoundMessagePtr CreateMessage() const{
      return NotFoundMessage::NewInstance("Not Found.");
    }
  public:
   ~NotFoundMessageTest() = default;
 };

 template<class M>
 class PaxosMessageTest : public MessageTest<M>{
  protected:
   PaxosMessageTest():
     MessageTest<M>(){}

   RawProposal CreateProposal() const{
     Timestamp timestamp = Clock::now();
     UUID id = UUID();
     UUID proposer = UUID();
     BlockPtr value = Block::Genesis();
     return RawProposal(timestamp, id, proposer, value->GetHeader());
   }

   std::shared_ptr<M> CreateMessage() const{
     return M::NewInstance(CreateProposal());
   }
  public:
   ~PaxosMessageTest() = default;
 };

 class PrepareMessageTest : public PaxosMessageTest<PrepareMessage>{
  protected:
   PrepareMessageTest():
    PaxosMessageTest<PrepareMessage>(){}
  public:
   ~PrepareMessageTest() = default;
 };

 class PromiseMessageTest : public PaxosMessageTest<PromiseMessage>{
  protected:
   PromiseMessageTest():
    PaxosMessageTest<PromiseMessage>(){}
  public:
   ~PromiseMessageTest() = default;
 };

 class CommitMessageTest : public PaxosMessageTest<CommitMessage>{
  protected:
   CommitMessageTest():
    PaxosMessageTest<CommitMessage>(){}
  public:
   ~CommitMessageTest() = default;
 };

 class AcceptedMessageTest : public PaxosMessageTest<AcceptedMessage>{
  protected:
   AcceptedMessageTest():
    PaxosMessageTest<AcceptedMessage>(){}
  public:
   ~AcceptedMessageTest() = default;
 };

 class RejectedMessageTest : public PaxosMessageTest<RejectedMessage>{
  protected:
   RejectedMessageTest():
    PaxosMessageTest<RejectedMessage>(){}
  public:
   ~RejectedMessageTest() = default;
 };

 class NotSupportedMessageTest : public MessageTest<NotSupportedMessage>{
  protected:
   NotSupportedMessagePtr
   CreateMessage() const{
     return NotSupportedMessage::NewInstance("Not Supported");
   }

   NotSupportedMessageTest():
    MessageTest<NotSupportedMessage>(){}
  public:
   ~NotSupportedMessageTest() = default;
 };
}

#endif//TOKEN_TEST_MESSAGE_H