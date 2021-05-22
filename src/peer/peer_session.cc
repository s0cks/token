#include "miner.h"
#include "configuration.h"
#include "job/scheduler.h"
#include "job/synchronize.h"
#include "rpc/rpc_server.h"
#include "peer/peer_session.h"

#include "rpc/messages/rpc_message_paxos.h"
#include "rpc/messages/rpc_message_object.h"
#include "rpc/messages/rpc_message_version.h"
#include "rpc/messages/rpc_message_inventory.h"

// The peer session sends packets to a peer
// any response received is purely in relation to the sent packets
// this is for outbound packets
namespace token{
#define PEER_LOG(LevelName, Session) \
  LOG(LevelName) << "[peer-" << (Session)->GetID().ToStringAbbreviated() << "] "

#define LOG_HANDLER(LevelName) \
  LOG_SESSION(LevelName, GetSession())
#define DLOG_HANDLER(LevelName) \
  DLOG_SESSION(LevelName, GetSession())

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
    Hash nonce = Hash::GenerateNonce();
    UUID node_id = ConfigurationManager::GetNodeID();
    session->Send(rpc::VersionMessage::NewInstance(Clock::now(), rpc::ClientType::kNode, Version::CurrentVersion(), nonce, node_id, head));

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
      rpc::MessagePtr message = rpc::Message::From(session, buffer);
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
    session->Send(rpc::InventoryListMessage::Of(blk));
  }

  void PeerSession::OnPrepare(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal

    ProposalPtr proposal = miner->GetActiveProposal();
    rpc::MessageList responses;
    responses << rpc::PrepareMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnPromise(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal
    ProposalPtr proposal = miner->GetActiveProposal();

    rpc::MessageList responses;
    responses << rpc::PromiseMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnCommit(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal
    ProposalPtr proposal = miner->GetActiveProposal();

    rpc::MessageList responses;
    responses << rpc::CommitMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnAccepted(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal

    ProposalPtr proposal = miner->GetActiveProposal();
    rpc::MessageList responses;
    responses << rpc::AcceptedMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerSession::OnRejected(uv_async_t* handle){
    BlockMiner* miner = BlockMiner::GetInstance();
    auto session = (PeerSession*) handle->data;
    //TODO: check for active proposal

    ProposalPtr proposal = miner->GetActiveProposal();
    rpc::MessageList responses;
    responses << rpc::RejectedMessage::NewInstance(proposal);
    return session->Send(responses);
  }

  void PeerMessageHandler::OnVersionMessage(const rpc::VersionMessagePtr& msg){
    rpc::ClientType type = rpc::ClientType::kNode;
    UUID node_id = ConfigurationManager::GetNodeID();
    NodeAddress callback = GetServerCallbackAddress();
    Version version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    BlockPtr head = BlockChain::GetInstance()->GetHead();//TODO: optimize
    Hash nonce = Hash::GenerateNonce();
    Send(rpc::VerackMessage::NewInstance(type, node_id, callback, version, head->GetHeader(), nonce));
  }

  //TODO:
  // - nonce check
  // - state transition
  // - set remote <HEAD>
  void PeerMessageHandler::OnVerackMessage(const rpc::VerackMessagePtr& msg){
    DLOG_HANDLER(INFO) << "current time: " << FormatTimestampReadable(msg->GetTimestamp());
    DLOG_HANDLER(INFO) << "HEAD: " << msg->GetHead();

    BlockChainPtr chain = BlockChain::GetInstance();
//TODO:
//    if(IsConnecting()){
//      Peer info(msg->GetID(), msg->GetCallbackAddress());
//      DLOG_HANDLER(INFO) << "connected to peer: " << info;
//
//      SetInfo(info);
//      SetState(Session::kConnectedState);
//
//      BlockHeader local_head = chain->GetHead()->GetHeader();
//      BlockHeader remote_head = msg->GetHead();
//      if(local_head < remote_head){
//        DLOG_HANDLER(INFO) << "starting new synchronization job to resolve remote/HEAD: " << remote_head;
//        //auto job = new SynchronizeJob(this, chain, remote_head);
//        //TODO: schedule new job
//        return;
//      }
//
//      DLOG_HANDLER(INFO) << "skipping synchronization, " << info << "/HEAD == HEAD";
//    }
  }

  void PeerMessageHandler::OnPrepareMessage(const rpc::PrepareMessagePtr& msg){
    NOT_IMPLEMENTED(WARNING);
  }

  void PeerMessageHandler::OnPromiseMessage(const rpc::PromiseMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();//TODO: fix usage?
    if(!miner->HasActiveProposal()){
      DLOG_HANDLER(ERROR) << "there isn't an active proposal";
      return;
    }

    //TODO:
    // - sanity check proposals for equivalency
    // - sanity check proposal state
    ProposalPtr proposal = miner->GetActiveProposal();
    //TODO:proposal->CastVote(GetID(), msg);
  }

  void PeerMessageHandler::OnCommitMessage(const rpc::CommitMessagePtr& msg){
    NOT_IMPLEMENTED(ERROR);// should never happen
  }

  void PeerMessageHandler::OnAcceptsMessage(const rpc::AcceptsMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();//TODO: fix usage?
    if(!miner->HasActiveProposal()){
      DLOG_HANDLER(ERROR) << "there isn't an active proposal";
      return;
    }

    //TODO:
    // - sanity check proposals for equivalency
    // - sanity check proposal state
    ProposalPtr proposal = miner->GetActiveProposal();
    //TODO:proposal->CastVote(GetID(), msg);
  }

  void PeerMessageHandler::OnAcceptedMessage(const rpc::AcceptedMessagePtr& msg){
    NOT_IMPLEMENTED(ERROR);// should never happen
  }

  void PeerMessageHandler::OnRejectsMessage(const rpc::RejectsMessagePtr& msg){
    BlockMiner* miner = BlockMiner::GetInstance();
    if(!miner->HasActiveProposal()){
      DLOG_HANDLER(ERROR) << "there isn't an active proposal";
      return;
    }

    //TODO:
    // - sanity check proposals for equivalency
    // - sanity check proposal state
    ProposalPtr proposal = miner->GetActiveProposal();
    //TODO:proposal->CastVote(GetID(), msg);
  }

  void PeerMessageHandler::OnRejectedMessage(const rpc::RejectedMessagePtr& msg){
    NOT_IMPLEMENTED(ERROR);// should never happen
  }

  void PeerMessageHandler::OnGetDataMessage(const rpc::GetDataMessagePtr& msg){
    BlockChainPtr chain = BlockChain::GetInstance();

    DLOG_HANDLER(INFO) << "getting " << msg->GetNumberOfItems() << " items....";

    rpc::MessageList responses;
    for(auto& item : msg->items()){
      DLOG_HANDLER(INFO) << "resolving " << item << "....";
      ObjectPoolPtr pool = ObjectPool::GetInstance();
      Hash hash = item.GetHash();
      if(item.IsBlock()){
        BlockPtr block;
        if(chain->HasBlock(hash)){
          DLOG_HANDLER(INFO) << item << " was found in the block chain.";
          block = chain->GetBlock(hash);
        } else if(pool->HasBlock(hash)){
          DLOG_HANDLER(INFO) << item << " was found in the pool.";
          block = pool->GetBlock(hash);
        } else{
          LOG_HANDLER(WARNING) << "cannot find " << item << "!";
          responses << rpc::NotFoundMessage::NewInstance(item, "cannot find item");
          break;
        }

        responses << rpc::BlockMessage::NewInstance(block);
      } else if(item.IsTransaction()){
        if(!pool->HasTransaction(hash)){
          LOG_HANDLER(WARNING) << "cannot find " << item << "!";
          responses << rpc::NotFoundMessage::NewInstance(item, "cannot find item");
          break;
        }

        TransactionPtr tx = pool->GetTransaction(hash);
        responses << rpc::TransactionMessage::NewInstance(tx);
      }
    }

    Send(responses);
  }

  void PeerMessageHandler::OnBlockMessage(const rpc::BlockMessagePtr& msg){
    ObjectPoolPtr pool = ObjectPool::GetInstance();//TODO: refactor
    BlockPtr blk = msg->GetValue();
    Hash hash = blk->GetHash();
    pool->PutBlock(hash, blk);
    LOG(INFO) << "received block: " << hash;
  }

  void PeerMessageHandler::OnTransactionMessage(const rpc::TransactionMessagePtr& msg){
    NOT_IMPLEMENTED(FATAL);
  }

  void PeerMessageHandler::OnNotFoundMessage(const rpc::NotFoundMessagePtr& msg){
    NOT_IMPLEMENTED(FATAL);
  }

  void PeerMessageHandler::OnInventoryListMessage(const rpc::InventoryListMessagePtr& msg){
    InventoryItems& items = msg->items();

    InventoryItems needed;
    for(auto& item : items){
//TODO:
//      if(!ItemExists(item)){
//        needed << item;
//      }
    }

    DLOG_HANDLER(INFO) << "requesting " << needed.size() << "/" << items.size() << " items from peer....";
    Send(rpc::GetDataMessage::NewInstance(needed));
  }

  void PeerMessageHandler::OnGetBlocksMessage(const rpc::GetBlocksMessagePtr& msg){}
}