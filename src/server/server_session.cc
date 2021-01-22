#include <glog/logging.h>
#include "pool.h"
#include "message.h"
#include "server/server.h"
#include "server/server_session.h"
#include "peer/peer_session_manager.h"

#include "consensus/proposal.h"
#include "consensus/proposal_manager.h"

namespace Token{
  void ServerSession::HandleNotFoundMessage(ServerSession* session, const NotFoundMessagePtr& msg){
    //TODO: implement HandleNotFoundMessage
    LOG(WARNING) << "not implemented";
    return;
  }

  void ServerSession::HandleVersionMessage(ServerSession* session, const VersionMessagePtr& msg){
    //TODO:
    // - state check
    // - version check
    // - echo nonce
    session->Send(VersionMessage::NewInstance(Server::GetID()));
  }

  void ServerSession::HandleVerackMessage(ServerSession* session, const VerackMessagePtr& msg){
    //TODO:
    // - verify nonce
    BlockPtr head = BlockChain::GetHead();
    session->Send(
      VerackMessage::NewInstance(
        ClientType::kNode,
        Server::GetID(),
        Server::GetCallbackAddress(),
        Version(),
        head->GetHeader(),
        Hash::GenerateNonce()));
    UUID id = msg->GetID();
    if(session->IsConnecting()){
      session->SetID(id);
      session->SetState(Session::State::kConnected);
      if(msg->IsNode()){
        NodeAddress paddr = msg->GetCallbackAddress();
        LOG(INFO) << "peer " << id << " connected from: " << paddr;
        PeerSessionManager::ConnectTo(paddr);
      }
    }
  }

  void ServerSession::HandleGetDataMessage(ServerSession* session, const GetDataMessagePtr& msg){
    std::vector<InventoryItem> items;
    if(!msg->GetItems(items)){
      LOG(WARNING) << "cannot get items from message";
      return;
    }

    std::vector<MessagePtr> response;
    for(auto& item : items){
      Hash hash = item.GetHash();
      LOG(INFO) << "searching for: " << hash;
      if(item.ItemExists()){
        if(item.IsBlock()){
          BlockPtr block;
          if(BlockChain::HasBlock(hash)){
            block = BlockChain::GetBlock(hash);
          } else if(ObjectPool::HasBlock(hash)){
            block = ObjectPool::GetBlock(hash);
          } else{
            LOG(WARNING) << "cannot find block: " << hash;
            response.push_back(NotFoundMessage::NewInstance());
            break;
          }
          response.push_back(BlockMessage::NewInstance(block));
        } else if(item.IsTransaction()){
          if(!ObjectPool::HasTransaction(hash)){
            LOG(WARNING) << "cannot find transaction: " << hash;
            response.push_back(NotFoundMessage::NewInstance());
            break;
          }

          TransactionPtr tx = ObjectPool::GetTransaction(hash);
          response.push_back(TransactionMessage::NewInstance(tx));//TODO: fix
        }
      } else{
        session->Send(NotFoundMessage::NewInstance());
        return;
      }
    }

    session->Send(response);
  }

  void ServerSession::HandlePrepareMessage(ServerSession* session, const PrepareMessagePtr& msg){
    if(ProposalManager::HasProposal()){
      LOG(WARNING) << "proposal manager already has a proposal.";
      session->Send(RejectedMessage::NewInstance(msg->GetProposal()));
      return;
    }

    ProposalPtr proposal = msg->GetProposal();
    // Stop miner..
    if(!ProposalManager::SetProposal(proposal)){
      LOG(WARNING) << "cannot set the active proposal to: " << proposal->ToString();
      session->Send(RejectedMessage::NewInstance(proposal));
      return;
    }

    session->Send(AcceptedMessage::NewInstance(proposal));
  }

  void ServerSession::HandlePromiseMessage(ServerSession* session, const PromiseMessagePtr& msg){
    if(!ProposalManager::HasProposal()){
      LOG(WARNING) << "received promise message for no active proposal of: " << msg->GetProposal()->ToString();
      session->Send(RejectedMessage::NewInstance(msg->GetProposal()));
      return;
    }

    if(!ProposalManager::IsProposalFor(msg->GetProposal())){
      LOG(WARNING) << "received promise message for invalid proposal: " << msg->GetProposal()->ToString();
      session->Send(RejectedMessage::NewInstance(msg->GetProposal()));
      return;
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    if(!proposal->TransitionToPhase(Proposal::kVotingPhase)){
      session->Send(RejectedMessage::NewInstance(proposal));
      return;
    }

    session->Send(AcceptedMessage::NewInstance(proposal));
  }

  void ServerSession::HandleCommitMessage(ServerSession* session, const CommitMessagePtr& msg){
    ProposalPtr remote_proposal = msg->GetProposal();
    if(!ProposalManager::IsProposalFor(remote_proposal)){
      LOG(WARNING) << "received accepted message from peer for inactive proposal: " << remote_proposal->ToString();
      return;
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    if(proposal->GetProposer() == Server::GetID()){
      proposal->AcceptProposal(session->GetID().ToString());//TODO: remove ToString()
    } else{
      proposal->SetPhase(Proposal::kCommitPhase);
    }
  }

  void ServerSession::HandleAcceptedMessage(ServerSession* session, const AcceptedMessagePtr& msg){
    ProposalPtr remote_proposal = msg->GetProposal();
    if(!ProposalManager::IsProposalFor(remote_proposal)){
      LOG(WARNING) << "received accepted message from peer for inactive proposal: " << remote_proposal->ToString();
      return;
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    proposal->AcceptProposal(session->GetID().ToString()); //TODO: remove ToString()
  }

  void ServerSession::HandleRejectedMessage(ServerSession* session, const RejectedMessagePtr& msg){
    ProposalPtr remote_proposal = msg->GetProposal();
    if(!ProposalManager::IsProposalFor(remote_proposal)){
      LOG(WARNING) << "received accepted message from peer for inactive proposal: " << remote_proposal->ToString();
      return;
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    proposal->RejectProposal(session->GetID().ToString()); //TODO: remove ToString()
  }

  void ServerSession::HandleBlockMessage(ServerSession* session, const BlockMessagePtr& msg){
    BlockPtr blk = msg->GetValue();
    Hash hash = blk->GetHash();
    if(!ObjectPool::HasBlock(hash)){
      ObjectPool::PutBlock(hash, blk);
      //TODO: Server::Broadcast(InventoryMessage::NewInstance(blk));
    }
  }

  void ServerSession::HandleTransactionMessage(ServerSession* session, const TransactionMessagePtr& msg){
    TransactionPtr tx = msg->GetValue();
    Hash hash = tx->GetHash();

    LOG(INFO) << "received transaction: " << hash;
    if(!ObjectPool::HasTransaction(hash)){
      ObjectPool::PutTransaction(hash, tx);
    }
  }

  void ServerSession::HandleInventoryMessage(ServerSession* session, const InventoryMessagePtr& msg){
    std::vector<InventoryItem> items;
    if(!msg->GetItems(items)){
      LOG(WARNING) << "couldn't get items from inventory message";
      return;
    }

    std::vector<InventoryItem> needed;
    for(auto& item : items){
      if(!item.ItemExists()) needed.push_back(item);
    }

    LOG(INFO) << "downloading " << needed.size() << "/" << items.size() << " items from inventory....";
    if(!needed.empty()) session->Send(GetDataMessage::NewInstance(needed));
  }

  void ServerSession::HandleGetBlocksMessage(ServerSession* session, const GetBlocksMessagePtr& msg){
    Hash start = msg->GetHeadHash();
    Hash stop = msg->GetStopHash();

    std::vector<InventoryItem> items;
    if(stop.IsNull()){
      intptr_t amt = std::min(GetBlocksMessage::kMaxNumberOfBlocks, BlockChain::GetHead()->GetHeight());
      LOG(INFO) << "sending " << (amt + 1) << " blocks...";

      BlockPtr start_block = BlockChain::GetBlock(start);
      BlockPtr stop_block = BlockChain::GetBlock(start_block->GetHeight() > amt ? start_block->GetHeight() + amt : amt);

      for(int64_t idx = start_block->GetHeight() + 1;
        idx <= stop_block->GetHeight();
        idx++){
        BlockPtr block = BlockChain::GetBlock(idx);
        LOG(INFO) << "adding " << block;
        items.push_back(InventoryItem(block));
      }
    }

    session->Send(InventoryMessage::NewInstance(items));
  }

  void ServerSession::HandleGetPeersMessage(ServerSession* session, const GetPeersMessagePtr& msg){
    LOG(INFO) << "getting peers....";
    PeerList peers;
    /*
    TODO:
        if(!Server::GetPeers(peers)){
            LOG(ERROR) << "couldn't get list of peers from the server.";
            return;
        }
     */

    session->Send(PeerListMessage::NewInstance(peers));
  }

  void ServerSession::HandlePeerListMessage(ServerSession* session, const PeerListMessagePtr& msg){
    //TODO: implement ServerSession::HandlePeerListMessage(HandleMessageTask*);
  }
}