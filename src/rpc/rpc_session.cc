#include <glog/logging.h>
#include "rpc/rpc_message.h"
#include "rpc/rpc_server.h"

#include "pool.h"
#include "miner.h"

#include "peer/peer_session_manager.h"

#include "consensus/proposal.h"
#include "consensus/proposal_manager.h"

// The server session receives packets sent from peers
// this is for inbound packets
namespace token{
#define NOT_IMPLEMENTED \
  SESSION_LOG(ERROR, this) << "not implemented!"

  //TODO:
  // - state check
  // - version check
  // - echo nonce
  void ServerSession::OnVersionMessage(const VersionMessagePtr& msg){
    // upon receiving a VersionMessage from a new client, respond w/ a VersionMessage
    // to initiate the connection handshake
    UUID id = ConfigurationManager::GetID(TOKEN_CONFIGURATION_NODE_ID);
    Send(VersionMessage::NewInstance(id));
  }

  //TODO:
  // - verify nonce
  void ServerSession::OnVerackMessage(const VerackMessagePtr& msg){
    // upon receiving a VerackMessage from a new client, respond w/ a VerackMessage
    // to finish the connection handshake
    ClientType type = ClientType::kNode;
    UUID node_id = ConfigurationManager::GetID(TOKEN_CONFIGURATION_NODE_ID);
    NodeAddress callback = LedgerServer::GetCallbackAddress();
    Version version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    BlockPtr head = BlockChain::GetHead(); //TODO: optimize
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
          BlockPtr block;
          if(BlockChain::HasBlock(hash)){
            block = BlockChain::GetBlock(hash);
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

  void ServerSession::OnPrepareMessage(const PrepareMessagePtr& msg){
    // upon receiving a PrepareMessage from a Peer:
    // - if a proposal is active AND the doesn't match the requested proposal, send a rejection.
    //  - pause local BlockMiner
    //  - set the current proposal
    //  - send an acceptance
    ProposalPtr proposal = msg->GetProposal();
    if(!BlockMiner::Pause())
      return Send(RejectedMessage::NewInstance(proposal));
    if(!ProposalManager::SetProposal(proposal))
      return Send(RejectedMessage::NewInstance(proposal));
    return Send(AcceptedMessage::NewInstance(proposal));
  }

  void ServerSession::OnPromiseMessage(const PromiseMessagePtr& msg){
    // upon receiving a PromiseMessage from a Peer:
    //  - if a proposal isn't active, send a rejection.
    //  - if the current proposal doesn't match the requested proposal, send a rejection.
    //  - transition the current proposal to kPromisePhase, send a rejection if this fails.
    //  - send an acceptance
    if(!ProposalManager::IsProposalFor(msg->GetProposal()))
      return Send(RejectedMessage::NewInstance(msg->GetProposal()));
    ProposalPtr proposal = ProposalManager::GetProposal();
    if(!proposal->TransitionToPhase(Proposal::kVotingPhase))
      return Send(RejectedMessage::NewInstance(proposal));
    return Send(AcceptedMessage::NewInstance(proposal));
  }

  void ServerSession::OnCommitMessage(const CommitMessagePtr& msg){
    // upon receiving a CommitMessage from a peer:
    //  - if a proposal isn't active, send a rejection.
    //  - if the current proposal doesn't match the requested proposal, send a rejection.
    //  - transition the current proposal to kCommitPhase, send a rejection if this fails.
    //  - commit the proposal - send a rejection if this fails.
    //  - send an acceptance.
    if(!ProposalManager::IsProposalFor(msg->GetProposal()))
      return Send(RejectedMessage::NewInstance(msg->GetProposal()));
    ProposalPtr proposal = ProposalManager::GetProposal();
    if(!proposal->TransitionToPhase(Proposal::kCommitPhase))
      return Send(RejectedMessage::NewInstance(proposal));
    if(!BlockMiner::Commit(proposal))
      return Send(RejectedMessage::NewInstance(proposal));
    return Send(AcceptedMessage::NewInstance(proposal));
  }

  void ServerSession::OnAcceptedMessage(const AcceptedMessagePtr& msg){
    // upon receiving an AcceptedMessage from a peer:
    //  - if a proposal isn't active, send a rejection.
    //  - if the current proposal doesn't match the requested proposal, send a rejection.
    //  - register sender in the accepted list for the current proposal
    if(!ProposalManager::IsProposalFor(msg->GetProposal()))
      return Send(RejectedMessage::NewInstance(msg->GetProposal()));
    ProposalPtr proposal = ProposalManager::GetProposal();
    proposal->AcceptProposal(GetUUID().ToString());
  }

  void ServerSession::OnRejectedMessage(const RejectedMessagePtr& msg){
    // upon receiving an AcceptedMessage from a peer:
    //  - if a proposal isn't active, send a rejection.
    //  - if the current proposal doesn't match the requested proposal, send a rejection.
    //  - register sender in the rejected list for the current proposal
    if(!ProposalManager::IsProposalFor(msg->GetProposal()))
      return Send(RejectedMessage::NewInstance(msg->GetProposal()));
    ProposalPtr proposal = ProposalManager::GetProposal();
    proposal->AcceptProposal(GetUUID().ToString());
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

  void ServerSession::OnGetBlocksMessage(const GetBlocksMessagePtr& msg){
    Hash start = msg->GetHeadHash();
    Hash stop = msg->GetStopHash();

    std::vector<InventoryItem> items;
    if(stop.IsNull()){
      intptr_t amt = std::min(GetBlocksMessage::kMaxNumberOfBlocks, BlockChain::GetHead()->GetHeight());
      SESSION_LOG(INFO, this) << "sending " << (amt + 1) << " blocks...";

      BlockPtr start_block = BlockChain::GetBlock(start);
      BlockPtr stop_block = BlockChain::GetBlock(start_block->GetHeight() > amt ? start_block->GetHeight() + amt : amt);

      for(int64_t idx = start_block->GetHeight() + 1;
        idx <= stop_block->GetHeight();
        idx++){
        BlockPtr block = BlockChain::GetBlock(idx);
        SESSION_LOG(INFO, this) << "adding " << block;
        items.push_back(InventoryItem(block));
      }
    }

    Send(InventoryMessage::NewInstance(items));
  }

  void ServerSession::OnNotFoundMessage(const NotFoundMessagePtr& msg){
    NOT_IMPLEMENTED; //TODO: implement ServerSession::OnNotFoundMessage
  }
}