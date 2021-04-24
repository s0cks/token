#include <glog/logging.h>
#include "rpc/rpc_message.h"
#include "rpc/rpc_server.h"

#include "pool.h"
#include "miner.h"

#include "peer/peer_session_manager.h"

// The server session receives packets sent from peers
// this is for inbound packets
namespace token{
  //TODO:
  // - state check
  // - version check
  // - echo nonce
  void ServerSession::OnVersionMessage(const VersionMessagePtr& msg){
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
  void ServerSession::OnVerackMessage(const VerackMessagePtr& msg){
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
    if(IsConnecting()){
      SetUUID(id);
      SetState(Session::kConnectedState);
      if(msg->IsNode()){
        NodeAddress paddr = msg->GetCallbackAddress();
        DLOG_SESSION(INFO, this) << "peer " << id << " connected from " << paddr;
        PeerSessionManager::ConnectTo(paddr);
      }
    }
  }

  void ServerSession::OnGetDataMessage(const GetDataMessagePtr& msg){
    BlockChainPtr chain = GetChain();
    ObjectPoolPtr pool = GetPool();

    RpcMessageList responses;
    for(auto& item : msg->items()){
      Hash hash = item.GetHash();
      DLOG_SESSION(INFO, this) << "searching for " << item << "....";
      if(ItemExists(item)){
        if(item.IsBlock()){
          BlockPtr block;
          if(chain->HasBlock(hash)){
            block = chain->GetBlock(hash);
          } else if(pool->HasBlock(hash)){
            block = pool->GetBlock(hash);
          } else{
            DLOG_SESSION(WARNING, this) << "couldn't find: " << item;
            responses << NotFoundMessage::NewInstance(item);
            break;
          }

          responses << BlockMessage::NewInstance(block);
        } else if(item.IsTransaction()){
          if(!pool->HasTransaction(hash)){
            DLOG_SESSION(WARNING, this) << "couldn't find: " << item;
            responses << NotFoundMessage::NewInstance(item);
            break;
          }

          TransactionPtr tx = pool->GetTransaction(hash);
          responses << TransactionMessage::NewInstance(tx);
        }
      } else{
        responses << NotFoundMessage::NewInstance(item);
      }
    }

    return Send(responses);
  }

#define REJECT_PROPOSAL(Proposal, LevelName, Message)({ \
  DLOG_SESSION(LevelName, this) << "rejecting proposal " << (Proposal) << ": " << (Message); \
  responses << RejectedMessage::NewInstance((Proposal));\
  return Send(responses);                               \
})

#define ACCEPT_PROPOSAL(Proposal, LevelName)({ \
  DLOG_SESSION(LevelName, this) << "accepting proposal: " << (Proposal); \
  responses << AcceptedMessage::NewInstance(proposal);                  \
  return Send(responses);                      \
})

#define PROMISE_PROPOSAL(Proposal, LevelName)({ \
  DLOG_SESSION(LevelName, this) << "promising proposal: " << (Proposal); \
  responses << PromiseMessage::NewInstance((Proposal));                   \
  return Send(responses);                       \
})

  void ServerSession::OnPrepareMessage(const PrepareMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();
    RpcMessageList responses;

    // check that the miner has an active proposal
    if(miner->HasActiveProposal())
      REJECT_PROPOSAL(msg->GetProposal(), ERROR, "there is already an active proposal.");
    // set the current proposal to the requested proposal
    if(!miner->RegisterNewProposal(msg->GetProposal()))
      REJECT_PROPOSAL(msg->GetProposal(), ERROR, "cannot set active proposal.");

    // pause the block miner
    if(!miner->Pause())
      REJECT_PROPOSAL(msg->GetProposal(), ERROR, "cannot pause the block miner.");

    // request block if it doesn't exist
    ObjectPoolPtr pool = ObjectPool::GetInstance();//TODO: refactor
    Hash hash = msg->GetProposal().GetValue().GetHash();
    if(!pool->HasBlock(hash)){
      DLOG_SESSION(INFO, this) << "couldn't find proposed block in pool, requesting: " << hash;
      responses << GetDataMessage::ForBlock(hash);
    }

    PROMISE_PROPOSAL(msg->GetProposal(), INFO);
  }

  void ServerSession::OnPromiseMessage(const PromiseMessagePtr& msg){
    NOT_IMPLEMENTED(WARNING);
  }

  void ServerSession::OnCommitMessage(const CommitMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();
    if(!miner->HasActiveProposal()){
      DLOG_SESSION(ERROR, this) << "there is no active proposal";
      return;
    }

    //TODO:
    // - sanity check the proposals for equivalency
    // - sanity check the proposal state
    ProposalPtr proposal = miner->GetActiveProposal();
    Send(AcceptsMessage::NewInstance(proposal));
  }

  void ServerSession::OnAcceptsMessage(const AcceptsMessagePtr& msg){
    NOT_IMPLEMENTED(ERROR); // should never happen
  }

  void ServerSession::OnAcceptedMessage(const AcceptedMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();
    if(!miner->HasActiveProposal()){
      DLOG_SESSION(ERROR, this) << "there is no active proposal";
      return;
    }

    //TODO:
    // - sanity check the proposals for equivalency
    // - sanity check the proposal state
    miner->SendAccepted();
  }

  void ServerSession::OnRejectsMessage(const RejectsMessagePtr& msg){
    NOT_IMPLEMENTED(ERROR); // should never happen
  }

  void ServerSession::OnRejectedMessage(const RejectedMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();
    if(!miner->HasActiveProposal()){
      DLOG_SESSION(ERROR, this) << "there is no active proposal";
      return;
    }

    //TODO:
    // - sanity check the proposals for equivalency
    // - sanity check the proposal state
    miner->SendRejected();
  }

  void ServerSession::OnBlockMessage(const BlockMessagePtr& msg){
    ObjectPoolPtr pool = GetPool();

    BlockPtr blk = msg->GetValue();
    Hash hash = blk->GetHash();

    DLOG_SESSION(INFO, this) << "received: " << blk->ToString();
    if(!pool->HasBlock(hash)){
      pool->PutBlock(hash, blk);
      //TODO: Server::Broadcast(InventoryMessage::FromParent(blk));
    }
  }

  void ServerSession::OnTransactionMessage(const TransactionMessagePtr& msg){
    ObjectPoolPtr pool = GetPool();

    TransactionPtr tx = msg->GetValue();
    Hash hash = tx->GetHash();

    DLOG_SESSION(INFO, this) << "received: " << tx->ToString();
    if(!pool->HasTransaction(hash)){
      pool->PutTransaction(hash, tx);
    }
  }

  void ServerSession::OnInventoryListMessage(const InventoryListMessagePtr& msg){
    InventoryItems& items = msg->items();
    DLOG_SESSION(INFO, this) << "received an inventory of " << items.size() << " items, resolving....";

    InventoryItems needed;
    for(auto& item : items){
      if(!ItemExists(item)){
        DLOG_SESSION(INFO, this) << item << " wasn't found, requesting....";
        needed << item;
      }
    }

    DLOG_SESSION(INFO, this) << "resolving " << needed.size() << "/" << items.size() << " items from peer....";
    if(!needed.empty())
      Send(GetDataMessage::NewInstance(needed));
  }

  //TODO: optimize
  void ServerSession::OnGetBlocksMessage(const GetBlocksMessagePtr& msg){
    BlockChainPtr chain = GetChain();

    Hash start = msg->GetHeadHash();
    Hash stop = msg->GetStopHash();

    InventoryItems items;
    if(stop.IsNull()){
      intptr_t amt = std::min(kMaxNumberOfBlocksForGetBlocksMessage, chain->GetHead()->GetHeight());
      DLOG_SESSION(INFO, this) << "sending " << (amt + 1) << " blocks...";

      BlockPtr start_block = chain->GetBlock(start);
      BlockPtr stop_block = chain->GetBlock(start_block->GetHeight() > amt ? start_block->GetHeight() + amt : amt);

      for(int64_t idx = start_block->GetHeight() + 1;
        idx <= stop_block->GetHeight();
        idx++){
        BlockPtr block = chain->GetBlock(idx);
        items << InventoryItem::Of(block);
      }
    }

    Send(InventoryListMessage::NewInstance(items));
  }

  void ServerSession::OnNotFoundMessage(const NotFoundMessagePtr& msg){
    NOT_IMPLEMENTED(WARNING);
  }
}