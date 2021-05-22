#include "rpc/test_rpc_message_paxos.h"

namespace token{
  TEST_F(PrepareMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::PrepareMessagePtr a = rpc::PrepareMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    rpc::PrepareMessagePtr b = rpc::PrepareMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(PrepareMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::PrepareMessagePtr a = rpc::PrepareMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    rpc::PrepareMessagePtr b = rpc::PrepareMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(PromiseMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::PromiseMessagePtr a = rpc::PromiseMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    rpc::PromiseMessagePtr b = rpc::PromiseMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(PromiseMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::PromiseMessagePtr a = rpc::PromiseMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    rpc::PromiseMessagePtr b = rpc::PromiseMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(CommitMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::CommitMessagePtr a = rpc::CommitMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    rpc::CommitMessagePtr b = rpc::CommitMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(CommitMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::CommitMessagePtr a = rpc::CommitMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    rpc::CommitMessagePtr b = rpc::CommitMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(AcceptsMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::AcceptsMessagePtr a = rpc::AcceptsMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    rpc::AcceptsMessagePtr b = rpc::AcceptsMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(AcceptsMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::AcceptsMessagePtr a = rpc::AcceptsMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    rpc::AcceptsMessagePtr b = rpc::AcceptsMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(AcceptedMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::AcceptedMessagePtr a = rpc::AcceptedMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    rpc::AcceptedMessagePtr b = rpc::AcceptedMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(AcceptedMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::AcceptedMessagePtr a = rpc::AcceptedMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    rpc::AcceptedMessagePtr b = rpc::AcceptedMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(RejectsMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::RejectsMessagePtr a = rpc::RejectsMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    rpc::RejectsMessagePtr b = rpc::RejectsMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(RejectsMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::RejectsMessagePtr a = rpc::RejectsMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    rpc::RejectsMessagePtr b = rpc::RejectsMessage::NewInstance(tmp);
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(RejectedMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::RejectedMessagePtr a = rpc::RejectedMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    rpc::RejectedMessagePtr b = rpc::RejectedMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(RejectedMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    rpc::RejectedMessagePtr a = rpc::RejectedMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    rpc::RejectedMessagePtr b = rpc::RejectedMessage::NewInstance(tmp);
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }
}