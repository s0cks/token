#ifdef TOKEN_ENABLE_SERVER

#include "miner.h"
#include "configuration.h"
#include "job/scheduler.h"
#include "job/synchronize.h"
#include "rpc/rpc_server.h"
#include "peer/peer_session.h"

// The peer session sends packets to a peer
// any response received is purely in relation to the sent packets
// this is for outbound packets
namespace token{
#define PEER_LOG(LevelName, Session) \
  LOG(LevelName) << "[peer-" << Session->GetID().ToStringAbbreviated() << "] "

#define CHECK_FOR_ACTIVE_PROPOSAL(Miner, Session, LevelName) \
  if(!(Miner)->HasActiveProposal()){     \
    SESSION_LOG(LevelName, Session) << "there is no active proposal."; \
    return;                                       \
  }

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
      PEER_LOG(ERROR, session) << "connect failure: " << uv_strerror(status);
      session->CloseConnection();
      return;
    }

    session->SetState(Session::kConnectingState);

    BlockPtr head = BlockChain::GetHead();
    UUID node_id = ConfigurationManager::GetNodeID();
    session->Send(VersionMessage::NewInstance(ClientType::kNode, node_id, head->GetHeader()));

    int err;
    if((err = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
      PEER_LOG(ERROR, session) << "read failure: " << uv_strerror(err);
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
#ifdef TOKEN_DEBUG
      PEER_LOG(INFO, session) << "received: " << message->ToString() << " (" << message->GetBufferSize() << "b)";
#endif//TOKEN_DEBUG
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
    BlockMiner* miner = BlockMiner::GetInstance();
    PeerSession* session = (PeerSession*) handle->data;
    CHECK_FOR_ACTIVE_PROPOSAL(miner, session, WARNING);

    ProposalPtr proposal = miner->GetActiveProposal();
    BlockHeader& blk = proposal->raw().GetValue();

    LOG(INFO) << "broadcasting newly discovered block hash: " << blk;
    session->Send(InventoryMessage::NewInstance(blk));
  }

  void PeerSession::OnPrepare(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    PeerSession* session = (PeerSession*) handle->data;
    CHECK_FOR_ACTIVE_PROPOSAL(miner, session, WARNING);

    ProposalPtr proposal = miner->GetActiveProposal();
    RpcMessageList responses;
    responses << PrepareMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnPromise(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    PeerSession* session = (PeerSession*) handle->data;
    CHECK_FOR_ACTIVE_PROPOSAL(miner, session, WARNING);
    ProposalPtr proposal = miner->GetActiveProposal();

    RpcMessageList responses;
    responses << PromiseMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnCommit(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    PeerSession* session = (PeerSession*) handle->data;
    CHECK_FOR_ACTIVE_PROPOSAL(miner, session, WARNING);

    ProposalPtr proposal = miner->GetActiveProposal();

    RpcMessageList responses;
    responses << CommitMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnAccepted(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    PeerSession* session = (PeerSession*) handle->data;
    CHECK_FOR_ACTIVE_PROPOSAL(miner, session, WARNING);

    ProposalPtr proposal = miner->GetActiveProposal();
    RpcMessageList responses;
    responses << AcceptedMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnRejected(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    PeerSession* session = (PeerSession*) handle->data;
    CHECK_FOR_ACTIVE_PROPOSAL(miner, session, WARNING);

    ProposalPtr proposal = miner->GetActiveProposal();
    RpcMessageList responses;
    responses << RejectedMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnVersionMessage(const VersionMessagePtr& msg){
    ClientType type = ClientType::kNode;
    UUID node_id = ConfigurationManager::GetNodeID();
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

  void PeerSession::OnPrepareMessage(const PrepareMessagePtr& msg){
    NOT_IMPLEMENTED(WARNING);
  }

  void PeerSession::OnPromiseMessage(const PromiseMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();

#ifdef TOKEN_DEBUG
    PEER_LOG(INFO, this) << "checking for active proposal....";
#endif//TOKEN_DEBUG
    CHECK_FOR_ACTIVE_PROPOSAL(miner, this, WARNING);

    ProposalPtr proposal = miner->GetActiveProposal();
    proposal->OnPromise();
  }

  void PeerSession::OnCommitMessage(const CommitMessagePtr& msg){
    NOT_IMPLEMENTED(WARNING);
  }

  void PeerSession::OnRejectedMessage(const RejectedMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();
    CHECK_FOR_ACTIVE_PROPOSAL(miner, this, WARNING);

    ProposalPtr proposal = miner->GetActiveProposal();
    switch(proposal->GetPhase()){
      case ProposalPhase::kPreparePhase:
        proposal->GetPhase1Quorum().RejectProposal(GetUUID());
        break;
      case ProposalPhase::kCommitPhase:
        proposal->GetPhase2Quorum().RejectProposal(GetUUID());
        break;
      default:
        SESSION_LOG(WARNING, this) << "invalid proposal state: " << proposal->GetPhase();
    }
  }

  void PeerSession::OnAcceptedMessage(const AcceptedMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();
    CHECK_FOR_ACTIVE_PROPOSAL(miner, this, WARNING);

    ProposalPtr proposal = miner->GetActiveProposal();
    Phase2Quorum& quorum = proposal->GetPhase2Quorum();
    quorum.AcceptProposal(GetUUID());
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