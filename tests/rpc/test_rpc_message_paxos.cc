#include "rpc/test_rpc_message_paxos.h"

namespace token{
  TEST_F(PrepareMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    PrepareMessagePtr a = PrepareMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    PrepareMessagePtr b = PrepareMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(PrepareMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    PrepareMessagePtr a = PrepareMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    PrepareMessagePtr b = PrepareMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(PromiseMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    PromiseMessagePtr a = PromiseMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    PromiseMessagePtr b = PromiseMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(PromiseMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    PromiseMessagePtr a = PromiseMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    PromiseMessagePtr b = PromiseMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(CommitMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    CommitMessagePtr a = CommitMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    CommitMessagePtr b = CommitMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(CommitMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    CommitMessagePtr a = CommitMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    CommitMessagePtr b = CommitMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(AcceptsMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    AcceptsMessagePtr a = AcceptsMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    AcceptsMessagePtr b = AcceptsMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(AcceptsMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    AcceptsMessagePtr a = AcceptsMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    AcceptsMessagePtr b = AcceptsMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(AcceptedMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    AcceptedMessagePtr a = AcceptedMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    AcceptedMessagePtr b = AcceptedMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(AcceptedMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    AcceptedMessagePtr a = AcceptedMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    AcceptedMessagePtr b = AcceptedMessage::NewInstance(tmp);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(RejectsMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    RejectsMessagePtr a = RejectsMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    RejectsMessagePtr b = RejectsMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(RejectsMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    RejectsMessagePtr a = RejectsMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    RejectsMessagePtr b = RejectsMessage::NewInstance(tmp);
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(RejectedMessageTest, EqualsTest){
    RawProposal proposal = CreateRandomProposal();
    RejectedMessagePtr a = RejectedMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    RejectedMessagePtr b = RejectedMessage::NewInstance(proposal);
    DLOG(INFO) << "b: " << b->ToString();
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }

  TEST_F(RejectedMessageTest, WriteMessageTest){
    RawProposal proposal = CreateRandomProposal();
    RejectedMessagePtr a = RejectedMessage::NewInstance(proposal);
    DLOG(INFO) << "a: " << a->ToString();
    BufferPtr tmp = Buffer::NewInstance(a->GetMessageSize());
    ASSERT_TRUE(a->WriteMessage(tmp));
    ASSERT_EQ(tmp->GetWrittenBytes(), a->GetMessageSize());
    RejectedMessagePtr b = RejectedMessage::NewInstance(tmp);
    ASSERT_TRUE(MessagesAreEqual(a, b));
  }
}