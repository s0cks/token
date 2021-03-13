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
    UUID node_id = ConfigurationManager::GetNodeID();
    Send(VersionMessage::NewInstance(node_id));
  }

  //TODO:
  // - verify nonce
  void ServerSession::OnVerackMessage(const VerackMessagePtr& msg){
    // upon receiving a VerackMessage from a new client, respond w/ a VerackMessage
    // to finish the connection handshake
    ClientType type = ClientType::kNode;
    UUID node_id = ConfigurationManager::GetNodeID();
    NodeAddress callback = LedgerServer::GetCallbackAddress();
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
        SESSION_LOG(INFO, this) << "peer " << id << " connected from: " << paddr;
        PeerSessionManager::ConnectTo(paddr);
      }
    }
  }

  void ServerSession::OnGetDataMessage(const GetDataMessagePtr& msg){
    std::vector<InventoryItem> items;
    if(!msg->GetItems(items)){
      SESSION_LOG(WARNING, this) << "cannot get items from message";
      return;
    }

    std::vector<RpcMessagePtr> response;
    for(auto& item : items){
      Hash hash = item.GetHash();
      SESSION_LOG(INFO, this) << "searching for: " << hash;
      if(item.ItemExists()){
        if(item.IsBlock()){
          BlockChain* chain = BlockChain::GetInstance();

          BlockPtr block;
          if(chain->HasBlock(hash)){
            block = chain->GetBlock(hash);
          } else if(ObjectPool::HasBlock(hash)){
            block = ObjectPool::GetBlock(hash);
          } else{
            SESSION_LOG(WARNING, this) << "cannot find block: " << hash;
            response.push_back(NotFoundMessage::NewInstance());
            break;
          }
          response.push_back(BlockMessage::NewInstance(block));
        } else if(item.IsTransaction()){
          if(!ObjectPool::HasTransaction(hash)){
            SESSION_LOG(WARNING, this) << "cannot find transaction: " << hash;
            response.push_back(NotFoundMessage::NewInstance());
            break;
          }

          TransactionPtr tx = ObjectPool::GetTransaction(hash);
          response.push_back(TransactionMessage::NewInstance(tx));//TODO: fix
        }
      } else{
        Send(NotFoundMessage::NewInstance());
        return;
      }
    }

    Send(response);
  }

#ifdef TOKEN_DEBUG
  #define REJECT_PROPOSAL(Proposal, LevelName, Message)({ \
    SESSION_LOG(LevelName, this) << "rejecting proposal " << (Proposal) << ": " << (Message); \
    responses << RejectedMessage::NewInstance((Proposal));\
    return Send(responses);                               \
  })
#else
  #define REJECT_PROPOSAL(Proposal, LevelName, Message)({ \
    responses << RejectedMessage::NewInstance((Proposal));\
    return Send(responses);                               \
  })
#endif//TOKEN_DEBUG

#define ACCEPT_PROPOSAL(Proposal, LevelName)({ \
  SESSION_LOG(LevelName, this) << "accepting proposal: " << (Proposal); \
  responses << AcceptedMessage::NewInstance(proposal);                  \
  return Send(responses);                      \
})

#define PROMISE_PROPOSAL(Proposal, LevelName)({ \
  SESSION_LOG(LevelName, this) << "promising proposal: " << (Proposal); \
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
    Hash hash = msg->GetProposal().GetValue().GetHash();
    if(!ObjectPool::HasBlock(hash)){
#ifdef TOKEN_DEBUG
      SESSION_LOG(INFO, this) << "cannot find proposed block in pool, requesting block: " << hash;
#endif//TOKEN_DEBUG
      responses << GetDataMessage::NewRequestForBlock(hash);
    }

    if(!miner->GetActiveProposal()->OnPrepare())
      REJECT_PROPOSAL(msg->GetProposal(), ERROR, "cannot invoke on-prepare");

    PROMISE_PROPOSAL(msg->GetProposal(), INFO);
  }

  void ServerSession::OnPromiseMessage(const PromiseMessagePtr& msg){
    NOT_IMPLEMENTED(WARNING);
  }

  void ServerSession::OnCommitMessage(const CommitMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();
    RpcMessageList responses;

    // check that the miner has a proposal active.
    if(!miner->HasActiveProposal())
      REJECT_PROPOSAL(msg->GetProposal(), ERROR, "no active proposal.");
    // check that the requested proposal matches the active proposal
    if(!miner->IsActiveProposal(msg->GetProposal()))
      REJECT_PROPOSAL(msg->GetProposal(), ERROR, "not the current proposal.");

    ProposalPtr proposal = miner->GetActiveProposal();
    if(!proposal->OnCommit())
      REJECT_PROPOSAL(msg->GetProposal(), ERROR, "cannot invoke on-commit");
  }

  void ServerSession::OnAcceptedMessage(const AcceptedMessagePtr& msg){
    NOT_IMPLEMENTED(WARNING);
  }

  void ServerSession::OnRejectedMessage(const RejectedMessagePtr& msg){
    NOT_IMPLEMENTED(WARNING);
  }

  void ServerSession::OnBlockMessage(const BlockMessagePtr& msg){
    BlockPtr blk = msg->GetValue();
    Hash hash = blk->GetHash();
    if(!ObjectPool::HasBlock(hash)){
      ObjectPool::PutBlock(hash, blk);
      //TODO: Server::Broadcast(InventoryMessage::FromParent(blk));
    }
  }

  void ServerSession::OnTransactionMessage(const TransactionMessagePtr& msg){
    TransactionPtr tx = msg->GetValue();
    Hash hash = tx->GetHash();

    SESSION_LOG(INFO, this) << "received transaction: " << hash;
    if(!ObjectPool::HasTransaction(hash)){
      ObjectPool::PutTransaction(hash, tx);
    }
  }

  void ServerSession::OnInventoryMessage(const InventoryMessagePtr& msg){
    std::vector<InventoryItem> items;
    if(!msg->GetItems(items)){
      SESSION_LOG(WARNING, this) << "couldn't get items from inventory message";
      return;
    }

    std::vector<InventoryItem> needed;
    for(auto& item : items){
      SESSION_LOG(INFO, this) << "checking for " << item;
      if(!item.ItemExists()){
        SESSION_LOG(INFO, this) << "not found, requesting....";
        needed.push_back(item);
      }
    }

    SESSION_LOG(INFO, this) << "downloading " << needed.size() << "/" << items.size() << " items from inventory....";
    if(!needed.empty()) Send(GetDataMessage::NewInstance(needed));
  }

  //TODO: optimize
  void ServerSession::OnGetBlocksMessage(const GetBlocksMessagePtr& msg){
    BlockChain* chain = BlockChain::GetInstance();

    Hash start = msg->GetHeadHash();
    Hash stop = msg->GetStopHash();

    std::vector<InventoryItem> items;
    if(stop.IsNull()){
      intptr_t amt = std::min(GetBlocksMessage::kMaxNumberOfBlocks, chain->GetHead()->GetHeight());
      SESSION_LOG(INFO, this) << "sending " << (amt + 1) << " blocks...";

      BlockPtr start_block = chain->GetBlock(start);
      BlockPtr stop_block = chain->GetBlock(start_block->GetHeight() > amt ? start_block->GetHeight() + amt : amt);

      for(int64_t idx = start_block->GetHeight() + 1;
        idx <= stop_block->GetHeight();
        idx++){
        BlockPtr block = chain->GetBlock(idx);
        SESSION_LOG(INFO, this) << "adding " << block;
        items.push_back(InventoryItem(block));
      }
    }

    Send(InventoryMessage::NewInstance(items));
  }

  void ServerSession::OnNotFoundMessage(const NotFoundMessagePtr& msg){
    NOT_IMPLEMENTED(WARNING);
  }
}