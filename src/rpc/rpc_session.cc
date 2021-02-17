#include <glog/logging.h>
#include "rpc/rpc_message.h"
#include "rpc/rpc_server.h"

#include "pool.h"
#include "miner.h"

#include "peer/peer_session_manager.h"

#include "consensus/proposal.h"
#include "consensus/proposal_manager.h"

namespace token{
#define NOT_IMPLEMENTED \
  SESSION_LOG(ERROR, this) << "not implemented!"

  void ServerSession::OnVersionMessage(const VersionMessagePtr& msg){
    //TODO:
    // - state check
    // - version check
    // - echo nonce
    UUID id = ConfigurationManager::GetID(TOKEN_CONFIGURATION_NODE_ID);
    Send(VersionMessage::NewInstance(id));
  }

  void ServerSession::OnVerackMessage(const VerackMessagePtr& msg){
    //TODO:
    // - verify nonce
    BlockPtr head = BlockChain::GetHead();
    Send(
      VerackMessage::NewInstance(
        ClientType::kNode,
        ConfigurationManager::GetID(TOKEN_CONFIGURATION_NODE_ID),
        LedgerServer::GetCallbackAddress(),
        Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION),
        BlockHeader(head),
        Hash::GenerateNonce())
    );
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
    ProposalPtr proposal = msg->GetProposal();
    if(!BlockMiner::Pause())
      return Send(RejectedMessage::NewInstance(proposal));
    if(!ProposalManager::SetProposal(proposal))
      return Send(RejectedMessage::NewInstance(proposal));
    return Send(AcceptedMessage::NewInstance(proposal));
  }

  void ServerSession::OnPromiseMessage(const PromiseMessagePtr& msg){
    if(!ProposalManager::IsProposalFor(msg->GetProposal()))
      return Send(RejectedMessage::NewInstance(msg->GetProposal()));
    ProposalPtr proposal = ProposalManager::GetProposal();
    if(!proposal->TransitionToPhase(Proposal::kVotingPhase))
      return Send(RejectedMessage::NewInstance(proposal));
    return Send(AcceptedMessage::NewInstance(proposal));
  }

  void ServerSession::OnCommitMessage(const CommitMessagePtr& msg){
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
    if(!ProposalManager::IsProposalFor(msg->GetProposal()))
      return Send(RejectedMessage::NewInstance(msg->GetProposal()));
    ProposalPtr proposal = ProposalManager::GetProposal();
    proposal->AcceptProposal(GetUUID().ToString());
  }

  void ServerSession::OnRejectedMessage(const RejectedMessagePtr& msg){
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
      //TODO: Server::Broadcast(InventoryMessage::NewInstance(blk));
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
      if(!item.ItemExists()) needed.push_back(item);
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