#include "rpc/rpc_server.h"

#include "miner.h"

namespace token{
  namespace rpc{
    #define DLOG_HANDLER(LevelName) DLOG_SESSION(LevelName, GetSession())

    //TODO:
    // - state check
    // - version check
    // - echo nonce
    void ServerMessageHandler::OnVersionMessage(const VersionMessagePtr &msg) {
      // upon receiving a VersionMessage from a new client, respond w/ a VersionMessage
      // to initiate the connection handshake
      Timestamp timestamp = Clock::now();
      ClientType client_type = ClientType::kNode;
      Version version = Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
      Hash nonce = msg->GetNonce();
      UUID node_id = ConfigurationManager::GetNodeID();
      BlockPtr head = GetChain()->GetHead();
      Send(VersionMessage::NewInstance(timestamp, client_type, version, nonce, node_id, head->GetHeader()));
    }

//TODO:
// - verify nonce
    void ServerMessageHandler::OnVerackMessage(const VerackMessagePtr &msg) {
      // upon receiving a VerackMessage from a new client, respond w/ a VerackMessage
      // to finish the connection handshake
      ClientType type = ClientType::kNode;
      UUID node_id = ConfigurationManager::GetNodeID();
      NodeAddress callback = GetServerCallbackAddress();
      Version version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
      BlockPtr head = BlockChain::GetInstance()->GetHead(); //TODO: optimize
      Hash nonce = Hash::GenerateNonce();
      Send(VerackMessage::NewInstance(type, node_id, callback, version, head->GetHeader(), nonce));

      UUID id = msg->GetID();
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

    void ServerMessageHandler::OnGetDataMessage(const GetDataMessagePtr &msg) {
      BlockChainPtr chain = GetChain();
      ObjectPoolPtr pool = GetPool();

      rpc::MessageList responses;
      for (auto &item : msg->items()) {
        Hash hash = item.GetHash();
        DLOG_HANDLER(INFO) << "searching for " << item << "....";
//TODO:
//    if (ItemExists(item)) {
//      if (item.IsBlock()) {
//        BlockPtr block;
//        if (chain->HasBlock(hash)) {
//          block = chain->GetBlock(hash);
//        } else if (pool->HasBlock(hash)) {
//          block = pool->GetBlock(hash);
//        } else {
//          DLOG_HANDLER(WARNING) << "couldn't find: " << item;
//          responses << NotFoundMessage::NewInstance(item);
//          break;
//        }
//        responses << BlockMessage::NewInstance(block);
//      } else if (item.IsTransaction()) {
//        if (!pool->HasTransaction(hash)) {
//          DLOG_HANDLER(WARNING) << "couldn't find: " << item;
//          responses << NotFoundMessage::NewInstance(item);
//          break;
//        }
//        TransactionPtr tx = pool->GetTransaction(hash);
//        responses << TransactionMessage::NewInstance(tx);
//      }
//    } else {
//      responses << NotFoundMessage::NewInstance(item);
//    }
      }

      return Send(responses);
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

    void ServerMessageHandler::OnPrepareMessage(const PrepareMessagePtr &msg) {
      BlockMiner *miner = BlockMiner::GetInstance();
      rpc::MessageList responses;

      // check that the miner has an active proposal
      if (miner->HasActiveProposal())
        REJECT_PROPOSAL(msg->GetProposal(), ERROR, "there is already an active proposal.");
      // set the current proposal to the requested proposal
      if (!miner->RegisterNewProposal(msg->GetProposal()))
        REJECT_PROPOSAL(msg->GetProposal(), ERROR, "cannot set active proposal.");

      // pause the block miner
      if (!miner->Pause())
        REJECT_PROPOSAL(msg->GetProposal(), ERROR, "cannot pause the block miner.");

      // request block if it doesn't exist
      ObjectPoolPtr pool = ObjectPool::GetInstance();//TODO: refactor
      Hash hash = msg->GetProposal().GetValue().GetHash();
      if (!pool->HasBlock(hash)) {
        DLOG_HANDLER(WARNING) << "couldn't find proposed block in pool, requesting: " << hash;
        responses << GetDataMessage::ForBlock(hash);
      }

      PROMISE_PROPOSAL(msg->GetProposal(), INFO);
    }

    void ServerMessageHandler::OnPromiseMessage(const PromiseMessagePtr &msg) {
      NOT_IMPLEMENTED(WARNING);
    }

    void ServerMessageHandler::OnCommitMessage(const CommitMessagePtr &msg) {
      BlockMiner *miner = BlockMiner::GetInstance();
      if (!miner->HasActiveProposal()) {
        DLOG_HANDLER(ERROR) << "there is no active proposal";
        return;
      }

      //TODO:
      // - sanity check the proposals for equivalency
      // - sanity check the proposal state
      ProposalPtr proposal = miner->GetActiveProposal();
      Send(AcceptsMessage::NewInstance(proposal));
    }

    void ServerMessageHandler::OnAcceptsMessage(const AcceptsMessagePtr &msg) {
      NOT_IMPLEMENTED(ERROR); // should never happen
    }

    void ServerMessageHandler::OnAcceptedMessage(const AcceptedMessagePtr &msg) {
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

    void ServerMessageHandler::OnRejectsMessage(const RejectsMessagePtr &msg) {
      NOT_IMPLEMENTED(ERROR); // should never happen
    }

    void ServerMessageHandler::OnRejectedMessage(const RejectedMessagePtr &msg) {
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

    void ServerMessageHandler::OnBlockMessage(const BlockMessagePtr &msg) {
      ObjectPoolPtr pool = GetPool();

      BlockPtr blk = msg->GetValue();
      Hash hash = blk->GetHash();

      DLOG_HANDLER(INFO) << "received: " << blk->ToString();
      if (!pool->HasBlock(hash)) {
        pool->PutBlock(hash, blk);
        //TODO: Server::Broadcast(InventoryMessage::FromParent(blk));
      }
    }

    void ServerMessageHandler::OnTransactionMessage(const TransactionMessagePtr &msg) {
      ObjectPoolPtr pool = GetPool();

      TransactionPtr tx = msg->GetValue();
      Hash hash = tx->GetHash();

      DLOG_HANDLER(INFO) << "received: " << tx->ToString();
      if (!pool->HasTransaction(hash)) {
        pool->PutTransaction(hash, tx);
      }
    }

    void ServerMessageHandler::OnInventoryListMessage(const InventoryListMessagePtr &msg) {
      InventoryItems &items = msg->items();
      DLOG_HANDLER(INFO) << "received an inventory of " << items.size() << " items, resolving....";

      InventoryItems needed;
      for (auto &item : items) {
//TODO:
//    if (!ItemExists(item)) {
//      DLOG_HANDLER(WARNING) << item << " wasn't found, requesting....";
//      needed << item;
//    }
      }

      DLOG_HANDLER(INFO) << "resolving " << needed.size() << "/" << items.size() << " items from peer....";
      if (!needed.empty())
        Send(GetDataMessage::NewInstance(needed));
    }

//TODO: optimize
    void ServerMessageHandler::OnGetBlocksMessage(const GetBlocksMessagePtr &msg) {
      BlockChainPtr chain = GetChain();

      Hash start = msg->GetHeadHash();
      Hash stop = msg->GetStopHash();

      InventoryItems items;
      if (stop.IsNull()) {
        intptr_t amt = std::min(kMaxNumberOfBlocksForGetBlocksMessage, chain->GetHead()->GetHeight());
        DLOG_HANDLER(INFO) << "sending " << (amt + 1) << " blocks...";

        BlockPtr start_block = chain->GetBlock(start);
        BlockPtr stop_block = chain->GetBlock(start_block->GetHeight() > amt ? start_block->GetHeight() + amt : amt);

        for (int64_t idx = start_block->GetHeight() + 1;
             idx <= stop_block->GetHeight();
             idx++) {
          BlockPtr block = chain->GetBlock(idx);
          items << InventoryItem::Of(block);
        }
      }

      Send(InventoryListMessage::NewInstance(items));
    }

    void ServerMessageHandler::OnNotFoundMessage(const NotFoundMessagePtr &msg) {
      NOT_IMPLEMENTED(WARNING);
    }
  }
}