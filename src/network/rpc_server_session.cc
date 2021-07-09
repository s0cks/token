#include "env.h"
#include "pool.h"
#include "miner.h"
#include "blockchain.h"

#include "network/rpc_server.h"
#include "network/rpc_server_session.h"

namespace token{
  namespace rpc{
    BlockChainPtr ServerSessionMessageHandler::GetChain() const{
      return ((ServerSession*)GetSession())->GetChain();
    }

    ObjectPoolPtr ServerSessionMessageHandler::GetPool() const{
      return ((ServerSession*)GetSession())->GetPool();
    }

#define DLOG_HANDLER(LevelName) DLOG_SESSION(LevelName, GetSession())

    //TODO:
    // - state check
    // - version check
    // - echo nonce
    void ServerSessionMessageHandler::OnVersionMessage(const VersionMessagePtr &msg) {
      // upon receiving a VersionMessage from a new client, respond w/ a VersionMessage
      // to initiate the connection handshake
      Timestamp timestamp = Clock::now();
      ClientType client_type = ClientType::kNode;
      Version version = Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
      Hash nonce = msg->nonce();
      UUID node_id = config::GetServerNodeID();
      BlockPtr head = GetChain()->GetHead();
      Send(VersionMessage::NewInstance(timestamp, client_type, version, nonce, node_id, head->GetHeader()));
    }

//TODO:
// - verify nonce
    void ServerSessionMessageHandler::OnVerackMessage(const VerackMessagePtr &msg) {
      // upon receiving a VerackMessage from a new client, respond w/ a VerackMessage
      // to finish the connection handshake
      ClientType type = ClientType::kNode;
      UUID node_id = config::GetServerNodeID();
      NodeAddress callback = env::GetServerCallbackAddress();
      Version version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
      BlockPtr head = GetChain()->GetHead();
      Hash nonce = Hash::GenerateNonce();
      Send(VerackMessage::NewInstance(Clock::now(), type, version, nonce, node_id, head->GetHeader(), callback));

      UUID id = msg->node_id();
//TODO:
//  if (IsConnecting()){
//    SetUUID(id);
//    SetState(Session::kConnectedState);
//    if (msg->IsNode()) {
//      NodeAddress paddr = msg->GetCallbackAddress();
//      DLOG_HANDLER(INFO) << "peer " << id << " connected from " << paddr;
//      PeerSessionManager::ConnectTo(paddr);
//    }
//  }
    }

#define REJECT_PROPOSAL(Proposal, LevelName, Message)({ \
  DLOG_HANDLER(LevelName) << "rejecting proposal " << (Proposal) << ": " << (Message); \
  responses << RejectedMessage::NewInstance((Proposal));\
  return Send(responses);                               \
})

#define ACCEPT_PROPOSAL(Proposal, LevelName)({ \
  DLOG_HANDLER(LevelName) << "accepting proposal: " << (Proposal); \
  responses << AcceptedMessage::NewInstance(proposal);                  \
  return Send(responses);                      \
})

#define PROMISE_PROPOSAL(Proposal, LevelName)({ \
  DLOG_HANDLER(LevelName) << "promising proposal: " << (Proposal); \
  responses << PromiseMessage::NewInstance((Proposal));                   \
  return Send(responses);                       \
})

    void ServerSessionMessageHandler::OnPrepareMessage(const PrepareMessagePtr &msg) {
      BlockMiner *miner = BlockMiner::GetInstance();
      rpc::MessageList responses;

      // check that the miner has an active proposal
      if (miner->HasActiveProposal())
        REJECT_PROPOSAL(msg->proposal(), ERROR, "there is already an active proposal.");
      // set the current proposal to the requested proposal
      if (!miner->RegisterNewProposal(msg->proposal()))
        REJECT_PROPOSAL(msg->proposal(), ERROR, "cannot set active proposal.");

      // pause the block miner
      if (!miner->Pause())
        REJECT_PROPOSAL(msg->proposal(), ERROR, "cannot pause the block miner.");

      // request block if it doesn't exist
      ObjectPoolPtr pool = ObjectPool::GetInstance();//TODO: refactor
      Hash hash = msg->proposal().value().hash();
      if (!pool->HasBlock(hash)) {
        DLOG_HANDLER(WARNING) << "couldn't find proposed block in pool, requesting: " << hash;
        //TODO: request block
      }

      PROMISE_PROPOSAL(msg->proposal(), INFO);
    }

    void ServerSessionMessageHandler::OnPromiseMessage(const PromiseMessagePtr &msg) {
      NOT_IMPLEMENTED(WARNING);
    }

    void ServerSessionMessageHandler::OnCommitMessage(const CommitMessagePtr &msg) {
      BlockMiner *miner = BlockMiner::GetInstance();
      if (!miner->HasActiveProposal()) {
        DLOG_HANDLER(ERROR) << "there is no active proposal";
        return;
      }

      //TODO:
      // - sanity check the proposals for equivalency
      // - sanity check the proposal state
      ProposalPtr proposal = miner->GetActiveProposal();
      Send(AcceptsMessage::NewInstance(proposal->raw()));
    }

    void ServerSessionMessageHandler::OnAcceptsMessage(const AcceptsMessagePtr &msg) {
      NOT_IMPLEMENTED(ERROR); // should never happen
    }

    void ServerSessionMessageHandler::OnAcceptedMessage(const AcceptedMessagePtr &msg) {
      BlockMiner *miner = BlockMiner::GetInstance();
      if (!miner->HasActiveProposal()) {
        DLOG_HANDLER(ERROR) << "there is no active proposal";
        return;
      }

      //TODO:
      // - sanity check the proposals for equivalency
      // - sanity check the proposal state
      miner->SendAccepted();
    }

    void ServerSessionMessageHandler::OnRejectedMessage(const RejectedMessagePtr &msg) {
      BlockMiner *miner = BlockMiner::GetInstance();
      if (!miner->HasActiveProposal()) {
        DLOG_HANDLER(ERROR) << "there is no active proposal";
        return;
      }

      //TODO:
      // - sanity check the proposals for equivalency
      // - sanity check the proposal state
      miner->SendRejected();
    }

    void ServerSessionMessageHandler::OnBlockMessage(const BlockMessagePtr &msg) {
      ObjectPoolPtr pool = GetPool();

      BlockPtr& value = msg->value();
      Hash hash = value->hash();
      DLOG_HANDLER(INFO) << "received: " << value->ToString();
      if (!pool->HasBlock(hash)) {
        pool->PutBlock(hash, value);
        //TODO: Server::Broadcast(InventoryMessage::FromParent(blk));
      }
    }

    void ServerSessionMessageHandler::OnTransactionMessage(const TransactionMessagePtr &msg) {
      ObjectPoolPtr pool = GetPool();

      UnsignedTransactionPtr& value = msg->value();
      Hash hash = value->hash();
      DLOG_HANDLER(INFO) << "received: " << value->ToString();
      if (!pool->HasUnsignedTransaction(hash)) {
        pool->PutUnsignedTransaction(hash, value);
      }
    }
  }
}