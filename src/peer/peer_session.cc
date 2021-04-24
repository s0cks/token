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
  LOG(LevelName) << "[peer-" << (Session)->GetID().ToStringAbbreviated() << "] "

  bool PeerSession::Connect(){
    //TODO: StartHeartbeatTimer();
    NodeAddress paddr = GetAddress();

    struct sockaddr_in addr{};
    uv_ip4_addr(paddr.GetAddress().c_str(), paddr.GetPort(), &addr);
    uv_connect_t conn;
    conn.data = this;

    DLOG_THREAD(INFO) << "creating connection to peer " << paddr << "....";
    VERIFY_UVRESULT(uv_tcp_connect(&conn, &handle_, (const struct sockaddr*)&addr, &PeerSession::OnConnect), LOG_THREAD(ERROR), "couldn't connect to peer");

    DLOG_THREAD(INFO) << "starting session loop....";
    VERIFY_UVRESULT(uv_run(GetLoop(), UV_RUN_DEFAULT), LOG_THREAD(ERROR), "couldn't run loop");
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
    auto session = (PeerSession*)conn->data;
    if(status != 0){
      PEER_LOG(ERROR, session) << "connect failure: " << uv_strerror(status);
      session->CloseConnection();
      return;
    }

    session->SetState(Session::kConnectingState);

    BlockPtr head = BlockChain::GetInstance()->GetHead();
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
    auto session = (PeerSession*)stream->data;
    if(nread == UV_EOF){
      DLOG_SESSION(ERROR, session) << "end of stream";
      return;
    } else if(nread < 0){
      DLOG_SESSION(ERROR, session) << "[" << nread << "] client read error: " << uv_strerror(nread);
      return;
    } else if(nread == 0){
      DLOG_SESSION(WARNING, session) << "zero message size received";
      return;
    } else if(nread > 65536){
      DLOG_SESSION(ERROR, session) << "received too large of a buffer (" << nread <<  " bytes)";
      return;
    }

    BufferPtr buffer = Buffer::From(buff->base, nread);
    do{
      RpcMessagePtr message = RpcMessage::From(session, buffer);
      DLOG_SESSION(INFO, session) << "received " << message->ToString() << " (" << message->GetBufferSize() << "b)";
      session->OnMessageRead(message);
    } while(buffer->GetReadBytes() < buffer->GetBufferSize());
    free(buff->base);
  }

  void PeerSession::OnDisconnect(uv_async_t* handle){
    auto session = (PeerSession*)handle->data;
    session->CloseConnection();
  }

  void PeerSession::OnWalk(uv_handle_t* handle, void* data){
    uv_close(handle, &OnClose);
  }

  void PeerSession::OnClose(uv_handle_t* handle){
    DLOG_THREAD(INFO) << "OnClose.";
  }

  void PeerSession::OnDiscovery(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal

    ProposalPtr proposal = miner->GetActiveProposal();
    BlockHeader& blk = proposal->raw().GetValue();
    session->Send(InventoryListMessage::Of(blk));
  }

  void PeerSession::OnPrepare(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal

    ProposalPtr proposal = miner->GetActiveProposal();
    RpcMessageList responses;
    responses << PrepareMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnPromise(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal
    ProposalPtr proposal = miner->GetActiveProposal();

    RpcMessageList responses;
    responses << PromiseMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnCommit(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal
    ProposalPtr proposal = miner->GetActiveProposal();

    RpcMessageList responses;
    responses << CommitMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnAccepted(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal

    ProposalPtr proposal = miner->GetActiveProposal();
    RpcMessageList responses;
    responses << AcceptedMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnRejected(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal

    ProposalPtr proposal = miner->GetActiveProposal();
    RpcMessageList responses;
    responses << RejectedMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnVersionMessage(const VersionMessagePtr& msg){
    ClientType type = ClientType::kNode;
    UUID node_id = ConfigurationManager::GetNodeID();
    NodeAddress callback = GetServerCallbackAddress();
    Version version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    BlockPtr head = BlockChain::GetInstance()->GetHead();//TODO: optimize
    Hash nonce = Hash::GenerateNonce();
    Send(VerackMessage::NewInstance(type, node_id, callback, version, head->GetHeader(), nonce));
  }

  //TODO:
  // - nonce check
  // - state transition
  // - set remote <HEAD>
  void PeerSession::OnVerackMessage(const VerackMessagePtr& msg){
    DLOG_SESSION(INFO, this) << "current time: " << FormatTimestampReadable(msg->GetTimestamp());
    DLOG_SESSION(INFO, this) << "HEAD: " << msg->GetHead();

    BlockChainPtr chain = BlockChain::GetInstance();
    if(IsConnecting()){
      Peer info(msg->GetID(), msg->GetCallbackAddress());
      DLOG_SESSION(INFO, this) << "connected to peer: " << info;

      SetInfo(info);
      SetState(Session::kConnectedState);

      BlockHeader local_head = chain->GetHead()->GetHeader();
      BlockHeader remote_head = msg->GetHead();
      if(local_head < remote_head){
        DLOG_SESSION(INFO, this) << "starting new synchronization job to resolve remote/HEAD: " << remote_head;
        //auto job = new SynchronizeJob(this, chain, remote_head);
        //TODO: schedule new job
        return;
      }

      DLOG_SESSION(INFO, this) << "skipping synchronization, " << info << "/HEAD == HEAD";
    }
  }

  void PeerSession::OnPrepareMessage(const PrepareMessagePtr& msg){
    NOT_IMPLEMENTED(WARNING);
  }

  void PeerSession::OnPromiseMessage(const PromiseMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();//TODO: fix usage?
    if(!miner->HasActiveProposal()){
      DLOG_SESSION(ERROR, this) << "there isn't an active proposal";
      return;
    }

    //TODO:
    // - sanity check proposals for equivalency
    // - sanity check proposal state
    ProposalPtr proposal = miner->GetActiveProposal();
    proposal->CastVote(GetID(), msg);
  }

  void PeerSession::OnCommitMessage(const CommitMessagePtr& msg){
    NOT_IMPLEMENTED(ERROR);// should never happen
  }

  void PeerSession::OnAcceptsMessage(const AcceptsMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();//TODO: fix usage?
    if(!miner->HasActiveProposal()){
      DLOG_SESSION(ERROR, this) << "there isn't an active proposal";
      return;
    }

    //TODO:
    // - sanity check proposals for equivalency
    // - sanity check proposal state
    ProposalPtr proposal = miner->GetActiveProposal();
    proposal->CastVote(GetID(), msg);
  }

  void PeerSession::OnAcceptedMessage(const AcceptedMessagePtr& msg){
    NOT_IMPLEMENTED(ERROR);// should never happen
  }

  void PeerSession::OnRejectsMessage(const RejectsMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();
    if(!miner->HasActiveProposal()){
      DLOG_SESSION(ERROR, this) << "there isn't an active proposal";
      return;
    }

    //TODO:
    // - sanity check proposals for equivalency
    // - sanity check proposal state
    ProposalPtr proposal = miner->GetActiveProposal();
    proposal->CastVote(GetID(), msg);
  }

  void PeerSession::OnRejectedMessage(const RejectedMessagePtr& msg){
    NOT_IMPLEMENTED(ERROR);// should never happen
  }

  void PeerSession::OnGetDataMessage(const GetDataMessagePtr& msg){
    BlockChainPtr chain = BlockChain::GetInstance();

#ifdef TOKEN_DEBUG
    LOG_SESSION(INFO, this) << "getting " << msg->GetNumberOfItems() << " items....";
#endif//TOKEN_DEBUG

    RpcMessageList responses;
    for(auto& item : msg->items()){
#ifdef TOKEN_DEBUG
      LOG_SESSION(INFO, this) << "resolving " << item << "....";
#endif//TOKEN_DEBUG

      ObjectPoolPtr pool = ObjectPool::GetInstance();
      Hash hash = item.GetHash();
      if(item.IsBlock()){
        BlockPtr block;
        if(chain->HasBlock(hash)){
#ifdef TOKEN_DEBUG
          LOG_SESSION(INFO, this) << item << " was found in the block chain.";
#endif//TOKEN_DEBUG

          block = chain->GetBlock(hash);
        } else if(pool->HasBlock(hash)){
#ifdef TOKEN_DEBUG
          LOG_SESSION(INFO, this) << item << " was found in the pool.";
#endif//TOKEN_DEBUG

          block = pool->GetBlock(hash);
        } else{
          LOG_SESSION(WARNING, this) << "cannot find " << item << "!";

          responses << NotFoundMessage::NewInstance(item, "cannot find item");
          break;
        }

        responses << BlockMessage::NewInstance(block);
      } else if(item.IsTransaction()){
        if(!pool->HasTransaction(hash)){
          LOG_SESSION(WARNING, this) << "cannot find " << item << "!";

          responses << NotFoundMessage::NewInstance(item, "cannot find item");
          break;
        }

        TransactionPtr tx = pool->GetTransaction(hash);
        responses << TransactionMessage::NewInstance(tx);
      }
    }

    Send(responses);
  }

  void PeerSession::OnBlockMessage(const BlockMessagePtr& msg){
    ObjectPoolPtr pool = ObjectPool::GetInstance();//TODO: refactor
    BlockPtr blk = msg->GetValue();
    Hash hash = blk->GetHash();
    pool->PutBlock(hash, blk);
    LOG(INFO) << "received block: " << hash;
  }

  void PeerSession::OnTransactionMessage(const TransactionMessagePtr& msg){

  }

  void PeerSession::OnNotFoundMessage(const NotFoundMessagePtr& msg){
    LOG(WARNING) << "(" << GetInfo() << "): " << msg->GetMessage();
  }

  void PeerSession::OnInventoryListMessage(const InventoryListMessagePtr& msg){
    InventoryItems& items = msg->items();

    InventoryItems needed;
    for(auto& item : items){
      if(!ItemExists(item)){
        needed << item;
      }
    }

#ifdef TOKEN_DEBUG
    LOG_SESSION(INFO, this) << "requesting " << needed.size() << "/" << items.size() << " items from peer....";
#endif//TOKEN_DEBUG
    Send(GetDataMessage::NewInstance(needed));
  }

  void PeerSession::OnGetBlocksMessage(const GetBlocksMessagePtr& msg){}
}