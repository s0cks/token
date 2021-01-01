#ifdef TOKEN_ENABLE_SERVER

#include "server.h"
#include "discovery.h"
#include "job/scheduler.h"
#include "job/synchronize.h"
#include "peer/peer_session.h"

namespace Token{
  bool PeerSession::Connect(){
    //TODO: session->StartHeartbeatTimer();
    NodeAddress paddr = GetAddress();

    struct sockaddr_in addr;
    uv_ip4_addr(paddr.GetAddress().c_str(), paddr.GetPort(), &addr);

    uv_connect_t conn;
    conn.data = this;

    int err;
    if((err = uv_tcp_connect(&conn, GetHandle(), (const struct sockaddr*) &addr, &PeerSession::OnConnect)) != 0){
      LOG(ERROR) << "couldn't connect to peer " << paddr << ": " << uv_strerror(err);
      return false;
    }

    uv_run(GetLoop(), UV_RUN_DEFAULT);
    return true;
  }

  bool PeerSession::Disconnect(){
    if(pthread_self() == thread_){
      uv_read_stop(GetStream());
      uv_stop(GetLoop());
      SetState(State::kDisconnected);
    } else{
      uv_async_send(&disconnect_);
    }
    //TODO: better error handling for PeerSession::Disconnect()
    return true;
  }

#define SESSION_OK_STATUS(Status, Message) \
    LOG(INFO) << (Message);             \
    session->SetStatus((Status));
#define SESSION_OK(Message) \
    SESSION_OK_STATUS(Session::kOk, (Message));
#define SESSION_WARNING_STATUS(Status, Message) \
    LOG(WARNING) << (Message);               \
    session->SetStatus((Status));
#define SESSION_WARNING(Message) \
    SESSION_WARNING_STATUS(Session::kWarning, (Message))
#define SESSION_ERROR_STATUS(Status, Message) \
    LOG(ERROR) << (Message);               \
    session->SetStatus((Status));
#define SESSION_ERROR(Message) \
    SESSION_ERROR_STATUS(Session::kError, (Message))

  void PeerSession::OnConnect(uv_connect_t* conn, int status){
    PeerSession* session = (PeerSession*) conn->data;
    if(status != 0){
      std::stringstream ss;
      ss << "client accept error: " << uv_strerror(status);
      SESSION_ERROR(ss.str());
      session->Disconnect();
      return;
    }

    session->SetState(Session::kConnecting);

    BlockPtr head = BlockChain::GetHead();
    session->Send(VersionMessage::NewInstance(ClientType::kNode, Server::GetID(), head->GetHeader()));
    if((status = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
      std::stringstream ss;
      ss << "client read error: " << uv_strerror(status);
      SESSION_ERROR(ss.str());
      session->Disconnect();
      return;
    }
  }

  void PeerSession::OnDisconnect(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    session->Disconnect();
  }

  void PeerSession::OnPrepare(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!BlockDiscoveryThread::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = BlockDiscoveryThread::GetProposal();
    session->Send(PrepareMessage::NewInstance(proposal));
  }

  void PeerSession::OnPromise(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!BlockDiscoveryThread::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = BlockDiscoveryThread::GetProposal();
    if(!proposal->IsProposal()){
      LOG(WARNING) << "cannot send another promise to the peer.";
      return;
    }
    session->Send(PromiseMessage::NewInstance(proposal));
  }

  void PeerSession::OnCommit(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!BlockDiscoveryThread::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = BlockDiscoveryThread::GetProposal();
    if(!proposal->IsCommit()){
      LOG(WARNING) << "cannot send another commit to the peer.";
      return;
    }
    session->Send(CommitMessage::NewInstance(proposal));
  }

  void PeerSession::OnAccepted(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!BlockDiscoveryThread::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = BlockDiscoveryThread::GetProposal();
    if(!proposal->IsProposal() && !proposal->IsVoting() && !proposal->IsCommit()){
      LOG(WARNING) << "cannot accept proposal #" << proposal->GetHeight() << " (" << proposal->GetPhase() << " ["
                   << proposal->GetResult() << "])";
      return;
    }
    session->Send(AcceptedMessage::NewInstance(proposal));
  }

  void PeerSession::OnRejected(uv_async_t* handle){
    PeerSession* session = (PeerSession*) handle->data;
    if(!BlockDiscoveryThread::HasProposal()){
      LOG(WARNING) << "there is no active proposal.";
      return;
    }

    ProposalPtr proposal = BlockDiscoveryThread::GetProposal();
    if(!proposal->IsProposal() && !proposal->IsVoting() && !proposal->IsCommit()){
      LOG(WARNING) << "cannot reject proposal #" << proposal->GetHeight() << " (" << proposal->GetPhase() << " ["
                   << proposal->GetResult() << "])";
      return;
    }
    session->Send(RejectedMessage::NewInstance(proposal));
  }

  void PeerSession::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
    PeerSession* session = (PeerSession*) stream->data;
    if(nread == UV_EOF){
      LOG(ERROR) << "client disconnected!";
      return;
    } else if(nread < 0){
      LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
      return;
    } else if(nread == 0){
      LOG(WARNING) << "zero message size received";
      return;
    } else if(nread >= 4096){
      LOG(ERROR) << "too large of a buffer";
      return;
    }

    uint32_t offset = 0;
    std::vector<MessagePtr> messages;

    BufferPtr& rbuff = session->GetReadBuffer();
    do{
      uint32_t mtype = rbuff->GetInt();
      intptr_t msize = rbuff->GetLong();

      switch(mtype){
#define DEFINE_DECODE(Name) \
                case Message::MessageType::k##Name##MessageType:{ \
                    MessagePtr msg = Name##Message::NewInstance(rbuff); \
                    messages.push_back(msg); \
                    break; \
                }
        FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
        case Message::MessageType::kUnknownMessageType:
        default:LOG(ERROR) << "unknown message type " << mtype << " of size " << msize;
          break;
      }

      offset += (msize + Message::kHeaderSize);
    } while((offset + Message::kHeaderSize) < nread);

    for(size_t idx = 0; idx < messages.size(); idx++){
      MessagePtr msg = messages[idx];
      switch(msg->GetMessageType()){
#define DEFINE_HANDLER_CASE(Name) \
            case Message::k##Name##MessageType: \
                session->Handle##Name##Message(session, std::static_pointer_cast<Name##Message>(msg)); \
                break;
        FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE);
#undef DEFINE_HANDLER_CASE
        case Message::kUnknownMessageType:
        default: //TODO: handle properly
          break;
      }
    }
    rbuff->Reset();
  }

  void PeerSession::HandleGetBlocksMessage(PeerSession* session, const GetBlocksMessagePtr& msg){}

  void PeerSession::HandleVersionMessage(PeerSession* session, const VersionMessagePtr& msg){
    BlockPtr head = BlockChain::GetHead();
    session->Send(VerackMessage::NewInstance(ClientType::kNode, Server::GetID(), head->GetHeader()));
  }

  void PeerSession::HandleVerackMessage(PeerSession* session, const VerackMessagePtr& msg){
    LOG(INFO) << "remote timestamp: " << GetTimestampFormattedReadable(msg->GetTimestamp());
    LOG(INFO) << "remote <HEAD>: " << msg->GetHead();

    //TODO: session->SetHead(msg->GetHead());
    if(session->IsConnecting()){
      session->SetInfo(Peer(msg->GetID(), msg->GetCallbackAddress()));
      LOG(INFO) << "connected to peer: " << session->GetInfo();
      session->SetState(Session::kConnected);

      BlockHeader local_head = BlockChain::GetHead()->GetHeader();
      BlockHeader remote_head = msg->GetHead();
      if(local_head == remote_head){
        LOG(INFO) << "skipping remote <HEAD> := " << remote_head;
      } else if(local_head < remote_head){
        SynchronizeJob* job = new SynchronizeJob(session, remote_head);
        if(!JobScheduler::Schedule(job)){
          LOG(WARNING) << "couldn't schedule SynchronizeJob";
          return;
        }
        session->Send(GetBlocksMessage::NewInstance());
      }
    }

    //TODO:
    // - nonce check
    // - state transition
  }

  void PeerSession::HandlePrepareMessage(PeerSession* session, const PrepareMessagePtr& msg){}
  void PeerSession::HandlePromiseMessage(PeerSession* session, const PromiseMessagePtr& msg){}
  void PeerSession::HandleCommitMessage(PeerSession* session, const CommitMessagePtr& msg){}
  void PeerSession::HandleAcceptedMessage(PeerSession* session, const AcceptedMessagePtr& msg){}
  void PeerSession::HandleRejectedMessage(PeerSession* session, const RejectedMessagePtr& msg){}

  void PeerSession::HandleGetDataMessage(PeerSession* session, const GetDataMessagePtr& msg){
    std::vector<InventoryItem> items;
    if(!msg->GetItems(items)){
      LOG(WARNING) << "cannot get items from message";
      return;
    }

    LOG(INFO) << "getting " << items.size() << " items....";
    std::vector<MessagePtr> response;
    for(auto& item : items){
      Hash hash = item.GetHash();
      LOG(INFO) << "resolving item : " << hash;
      if(item.IsBlock()){
        BlockPtr block;
        if(BlockChain::HasBlock(hash)){
          LOG(INFO) << "item " << hash << " found in block chain";
          block = BlockChain::GetBlock(hash);
        } else if(ObjectPool::HasObject(hash)){
          LOG(INFO) << "item " << hash << " found in block pool";
          block = ObjectPool::GetBlock(hash);
        } else{
          LOG(WARNING) << "cannot find block: " << hash;
          response.push_back(NotFoundMessage::NewInstance());
          break;
        }
        response.push_back(BlockMessage::NewInstance(block));
      } else if(item.IsTransaction()){
        if(!ObjectPool::HasObject(hash)){
          LOG(WARNING) << "cannot find transaction: " << hash;
          response.push_back(NotFoundMessage::NewInstance());
          break;
        }

        TransactionPtr tx = ObjectPool::GetTransaction(hash);
        response.push_back(TransactionMessage::NewInstance(tx));
      }
    }
    session->Send(response);
  }

  void PeerSession::HandleBlockMessage(PeerSession* session, const BlockMessagePtr& msg){
    BlockPtr blk = msg->GetValue();
    Hash hash = blk->GetHash();
    ObjectPool::PutObject(hash, blk);
    LOG(INFO) << "received block: " << hash;
  }

  void PeerSession::HandleTransactionMessage(PeerSession* session, const TransactionMessagePtr& msg){

  }

  void PeerSession::HandleUnclaimedTransactionMessage(PeerSession* session, const UnclaimedTransactionMessagePtr& msg){

  }

  void PeerSession::HandleNotFoundMessage(PeerSession* session, const NotFoundMessagePtr& msg){
    LOG(WARNING) << "(" << session->GetInfo() << "): " << msg->GetMessage();
  }

  void PeerSession::HandleInventoryMessage(PeerSession* session, const InventoryMessagePtr& msg){
    std::vector<InventoryItem> items;
    if(!msg->GetItems(items)){
      LOG(WARNING) << "couldn't get items from inventory message";
      return;
    }

    std::vector<InventoryItem> needed;
    for(auto& item : items){
      if(!item.ItemExists())
        needed.push_back(item);
    }

    LOG(INFO) << "downloading " << needed.size() << "/" << items.size() << " items from inventory....";
    session->Send(GetDataMessage::NewInstance(items));
  }

  void PeerSession::HandleGetPeersMessage(PeerSession* session, const GetPeersMessagePtr& msg){
    //TODO: implement PeerSession::HandleGetPeersMessage(HandleMessageTask*);
  }

  void PeerSession::HandlePeerListMessage(PeerSession* session, const PeerListMessagePtr& msg){
    //TODO: implement PeerSession::HandlePeerListMessage(HandleMessageTask*);
  }
}

#endif//TOKEN_ENABLE_SERVER