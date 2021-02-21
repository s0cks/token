#ifdef TOKEN_ENABLE_SERVER

#include "configuration.h"
#include "job/scheduler.h"
#include "job/synchronize.h"
#include "rpc/rpc_server.h"
#include "peer/peer_session.h"
#include "consensus/proposal.h"
#include "consensus/proposal_manager.h"

// The peer session sends packets to a peer
// any response received is purely in relation to the sent packets
// this is for outbound packets
namespace token{
  bool PeerSession::Connect(){
    //TODO: StartHeartbeatTimer();
    int err;
    NodeAddress paddr = GetAddress();

    struct sockaddr_in addr;
    uv_ip4_addr(paddr.GetAddress().c_str(), paddr.GetPort(), &addr);
    uv_connect_t conn;
    conn.data = this;

#ifdef TOKEN_DEBUG
    THREAD_LOG(INFO) << "creating connection to peer " << paddr << "....";
#endif//TOKEN_DEBUG
    if((err = uv_tcp_connect(&conn, &handle_, (const struct sockaddr*) &addr, &PeerSession::OnConnect)) != 0){
      THREAD_LOG(ERROR) << "couldn't connect to peer " << paddr << ": " << uv_strerror(err);
      return false;
    }

#ifdef TOKEN_DEBUG
    THREAD_LOG(INFO) << "starting session loop....";
#endif//TOKEN_DEBUG
    if((err = uv_run(GetLoop(), UV_RUN_DEFAULT)) != 0){
      THREAD_LOG(ERROR) << "couldn't run loop: " << uv_strerror(err);
      return false;
    }
    return true;
  }

  bool PeerSession::Disconnect(){
    int err;
    if((err = uv_async_send(&disconnect_)) != 0){
      LOG(WARNING) << "cannot send disconnect to peer: " << uv_strerror(err);
      return false;
    }
    return true;
  }

  void PeerSession::OnConnect(uv_connect_t* conn, int status){
    PeerSession* session = (PeerSession*)conn->data;
    if(status != 0){
      THREAD_LOG(ERROR) << "connect failure: " << uv_strerror(status);
      session->CloseConnection();
      return;
    }

    session->SetState(Session::kConnectingState);

    BlockPtr head = BlockChain::GetHead();
    UUID server_id = ConfigurationManager::GetID(TOKEN_CONFIGURATION_NODE_ID);
    session->Send(VersionMessage::NewInstance(ClientType::kNode, server_id, head->GetHeader()));

    int err;
    if((err = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
      THREAD_LOG(ERROR) << "read failure: " << uv_strerror(err);
      session->CloseConnection();
      return;
    }
  }

  void PeerSession::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
    PeerSession* session = (PeerSession*)stream->data;
    if(nread == UV_EOF){
      LOG(ERROR) << "client disconnected!";
      return;
    } else if(nread < 0){
      LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
      return;
    } else if(nread == 0){
      LOG(WARNING) << "zero message size received";
      return;
    } else if(nread > 65536){
      LOG(ERROR) << "too large of a buffer: " << nread;
      return;
    }

    BufferPtr buffer = Buffer::From(buff->base, nread);
    do{
      RpcMessagePtr message = RpcMessage::From(session, buffer);
      session->OnMessageRead(message);
    } while(buffer->GetReadBytes() < buffer->GetBufferSize());
    free(buff->base);
  }

  void PeerSession::OnDisconnect(uv_async_t* handle){
    PeerSession* session = (PeerSession*)handle->data;
    session->CloseConnection();
  }

  void PeerSession::OnWalk(uv_handle_t* handle, void* data){
    uv_close(handle, &OnClose);
  }

  void PeerSession::OnClose(uv_handle_t* handle){
    THREAD_LOG(INFO) << "on-close.";
  }

  void PeerSession::OnDiscovery(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!ProposalManager::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    BlockHeader blk = proposal->GetBlock();

    LOG(INFO) << "broadcasting newly discovered block hash: " << blk;
    session->Send(InventoryMessage::NewInstance(blk));
  }

  void PeerSession::OnPrepare(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!ProposalManager::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    LOG(INFO) << "sending new proposal: " << proposal->GetRaw();
    session->Send(PrepareMessage::NewInstance(proposal));
  }

  void PeerSession::OnPromise(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!ProposalManager::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    if(!proposal->IsVoting()){
      LOG(WARNING) << "cannot send another promise to the peer.";
      return;
    }
    session->Send(PromiseMessage::NewInstance(proposal));
  }

  void PeerSession::OnCommit(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!ProposalManager::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    if(!proposal->IsCommit()){
      LOG(WARNING) << "cannot send another commit to the peer.";
      return;
    }
    session->Send(CommitMessage::NewInstance(proposal));
  }

  void PeerSession::OnAccepted(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!ProposalManager::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    LOG(INFO) << "accepting " << session->GetInfo() << "'s proposal: " << proposal->ToString();
    session->Send(AcceptedMessage::NewInstance(proposal));
  }

  void PeerSession::OnRejected(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!ProposalManager::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    LOG(INFO) << "rejecting " << session->GetInfo() << "'s proposal: " << proposal->ToString();
    session->Send(RejectedMessage::NewInstance(proposal));
  }

  void PeerSession::OnVersionMessage(const VersionMessagePtr& msg){
    ClientType type = ClientType::kNode;
    UUID node_id = ConfigurationManager::GetID(TOKEN_CONFIGURATION_NODE_ID);
    NodeAddress callback = LedgerServer::GetCallbackAddress();
    Version version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    BlockPtr head = BlockChain::GetHead();//TODO: optimize
    Hash nonce = Hash::GenerateNonce();
    Send(VerackMessage::NewInstance(type, node_id, callback, version, head->GetHeader(), nonce));
  }

  //TODO:
  // - nonce check
  // - state transition
  // - set remote <HEAD>
  void PeerSession::OnVerackMessage(const VerackMessagePtr& msg){
#ifdef TOKEN_DEBUG
    SESSION_LOG(INFO, this) << "remote timestamp: " << FormatTimestampReadable(msg->GetTimestamp());
    SESSION_LOG(INFO, this) << "remote <HEAD>: " << msg->GetHead();
#endif//TOKEN_DEBUG

    if(IsConnecting()){
      Peer info(msg->GetID(), msg->GetCallbackAddress());
      SESSION_LOG(INFO, this) << "connected to peer: " << info;
      SetInfo(info);
      SetState(Session::kConnectedState);

      BlockHeader local_head = BlockChain::GetHead()->GetHeader();
      BlockHeader remote_head = msg->GetHead();
      if(local_head < remote_head){
#ifdef TOKEN_DEBUG
        SESSION_LOG(INFO, this) << "starting new SynchronizeJob to resolve remote/<HEAD> := " << remote_head;
#endif//TOKEN_DEBUG

        SynchronizeJob* job = new SynchronizeJob(this, remote_head);
        if(!JobScheduler::Schedule(job)){
          SESSION_LOG(WARNING, this) << "couldn't schedule new SynchronizeJob";
          return;
        }

        Send(GetBlocksMessage::NewInstance());
        return;
      }

#ifdef TOKEN_DEBUG
      SESSION_LOG(INFO, this) << info << "/<HEAD> == <HEAD>, skipping synchronization.";
#endif//TOKEN_DEBUG
    }
  }

  void PeerSession::OnPrepareMessage(const PrepareMessagePtr& msg){}
  void PeerSession::OnPromiseMessage(const PromiseMessagePtr& msg){}
  void PeerSession::OnCommitMessage(const CommitMessagePtr& msg){}
  void PeerSession::OnRejectedMessage(const RejectedMessagePtr& msg){}

  void PeerSession::OnAcceptedMessage(const AcceptedMessagePtr& msg){
    ProposalPtr remote_proposal = msg->GetProposal();
    if(!ProposalManager::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return Send(RejectedMessage::NewInstance(remote_proposal));
    }

    if(!ProposalManager::IsProposalFor(remote_proposal)){
      LOG(WARNING) << "active proposal is not: " << remote_proposal->ToString();
      return Send(RejectedMessage::NewInstance(remote_proposal));
    }

    ProposalPtr proposal = ProposalManager::GetProposal();
    proposal->AcceptProposal(GetID().ToString()); //TODO: fix cast?
  }

  void PeerSession::OnGetDataMessage(const GetDataMessagePtr& msg){
    std::vector<InventoryItem> items;
    if(!msg->GetItems(items)){
      LOG(WARNING) << "cannot get items from message";
      return;
    }

    LOG(INFO) << "getting " << items.size() << " items....";
    std::vector<RpcMessagePtr> response;
    for(auto& item : items){
      Hash hash = item.GetHash();
      LOG(INFO) << "resolving item : " << hash;
      if(item.IsBlock()){
        BlockPtr block;
        if(BlockChain::HasBlock(hash)){
          LOG(INFO) << "item " << hash << " found in block chain";
          block = BlockChain::GetBlock(hash);
        } else if(ObjectPool::HasBlock(hash)){
          LOG(INFO) << "item " << hash << " found in block pool";
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
        response.push_back(TransactionMessage::NewInstance(tx));
      }
    }
    Send(response);
  }

  void PeerSession::OnBlockMessage(const BlockMessagePtr& msg){
    BlockPtr blk = msg->GetValue();
    Hash hash = blk->GetHash();
    ObjectPool::PutBlock(hash, blk);
    LOG(INFO) << "received block: " << hash;
  }

  void PeerSession::OnTransactionMessage(const TransactionMessagePtr& msg){

  }

  void PeerSession::OnNotFoundMessage(const NotFoundMessagePtr& msg){
    LOG(WARNING) << "(" << GetInfo() << "): " << msg->GetMessage();
  }

  void PeerSession::OnInventoryMessage(const InventoryMessagePtr& msg){
    std::vector<InventoryItem> items;
    if(!msg->GetItems(items)){
      LOG(WARNING) << "couldn't get items from inventory message";
      return;
    }

    std::vector<InventoryItem> needed;
    for(auto& item : items){
      if(!item.ItemExists()){
        needed.push_back(item);
      }
    }

    LOG(INFO) << "downloading " << needed.size() << "/" << items.size() << " items from inventory....";
    Send(GetDataMessage::NewInstance(items));
  }

  void PeerSession::OnGetBlocksMessage(const GetBlocksMessagePtr& msg){}
}

#endif//TOKEN_ENABLE_SERVER