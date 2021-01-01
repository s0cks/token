#include <uuid/uuid.h>
#include <algorithm>
#include <random>
#include <condition_variable>

#include "pool.h"
#include "server.h"
#include "configuration.h"
#include "proposal.h"
#include "unclaimed_transaction.h"
#include "discovery.h"
#include "utils/crash_report.h"
#include "peer/peer_session_manager.h"

namespace Token{
  static pthread_t thread_;
  static uv_tcp_t handle_;
  static uv_async_t shutdown_;

  static std::mutex mutex_;
  static std::condition_variable cond_;
  static Server::State state_ = Server::State::kStopped;
  static Server::Status status_ = Server::Status::kOk;
  static UUID node_id_;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

  uv_tcp_t* Server::GetHandle(){
    return &handle_;
  }

  UUID Server::GetID(){
    LOCK_GUARD;
    return node_id_;
  }

  void Server::WaitForState(Server::State state){
    LOCK;
    while(state_ != state) WAIT;
    UNLOCK;
  }

  void Server::SetState(Server::State state){
    LOCK;
    state_ = state;
    UNLOCK;
    SIGNAL_ALL;
  }

  Server::State Server::GetState(){
    LOCK_GUARD;
    return state_;
  }

  void Server::SetStatus(Server::Status status){
    LOCK;
    status_ = status;
    UNLOCK;
    SIGNAL_ALL;
  }

  Server::Status Server::GetStatus(){
    LOCK_GUARD;
    return status_;
  }

  std::string Server::GetStatusMessage(){
    std::stringstream ss;
    LOCK_GUARD;
    ss << "[";
    switch(state_){
#define DEFINE_STATE_MESSAGE(Name) \
            case Server::k##Name: \
                ss << #Name; \
                break;
      FOR_EACH_SERVER_STATE(DEFINE_STATE_MESSAGE)
#undef DEFINE_STATE_MESSAGE
    }
    ss << "] ";

    switch(status_){
#define DEFINE_STATUS_MESSAGE(Name) \
            case Server::k##Name:{ \
                ss << #Name; \
                break; \
            }
      FOR_EACH_SERVER_STATUS(DEFINE_STATUS_MESSAGE)
#undef DEFINE_STATUS_MESSAGE
    }
    return ss.str();
  }

  void Server::OnWalk(uv_handle_t* handle, void* data){
    uv_close(handle, &OnClose);
  }

  void Server::OnClose(uv_handle_t* handle){}

  void Server::HandleTerminateCallback(uv_async_t* handle){
    SetState(Server::kStopping);
    uv_stop(handle->loop);

    int err;
    if((err = uv_loop_close(handle->loop)) == UV_EBUSY)
      uv_walk(handle->loop, &OnWalk, NULL);

    uv_run(handle->loop, UV_RUN_DEFAULT);
    if((err = uv_loop_close(handle->loop))){
      LOG(ERROR) << "failed to close server loop: " << uv_strerror(err);
    } else{
      LOG(INFO) << "server loop closed.";
    }
    SetState(Server::kStopped);
  }

  bool Server::Start(){
    if(!IsStopped()){
      LOG(WARNING) << "the server is already running.";
      return false;
    }
    return Thread::Start(&thread_, "Server", &HandleServerThread, 0);
  }

  bool Server::Stop(){
    if(!IsRunning())
      return true; // should we return false?
    uv_async_send(&shutdown_);
    return Thread::Stop(thread_);
  }

  void Server::HandleServerThread(uword parameter){
    LOG(INFO) << "starting server...";
    SetState(State::kStarting);
    uv_loop_t* loop = uv_loop_new();
    uv_async_init(loop, &shutdown_, &HandleTerminateCallback);

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", FLAGS_port, &addr);
    uv_tcp_init(loop, GetHandle());
    uv_tcp_keepalive(GetHandle(), 1, 60);

    int err;
    if((err = uv_tcp_bind(GetHandle(), (const struct sockaddr*) &addr, 0)) != 0){
      LOG(ERROR) << "couldn't bind the server: " << uv_strerror(err);
      goto exit;
    }

    if((err = uv_listen((uv_stream_t*) GetHandle(), 100, &OnNewConnection)) != 0){
      LOG(ERROR) << "server couldn't listen: " << uv_strerror(err);
      goto exit;
    }

    LOG(INFO) << "server " << GetID() << " listening @" << FLAGS_port;
    SetState(State::kRunning);
    uv_run(loop, UV_RUN_DEFAULT);
    LOG(INFO) << "server is stopping...";
    exit:
    pthread_exit(0);
  }

  void Server::OnNewConnection(uv_stream_t* stream, int status){
    if(status != 0){
      LOG(ERROR) << "connection error: " << uv_strerror(status);
      return;
    }

    ServerSession* session = ServerSession::NewInstance(stream->loop);
    session->SetState(Session::kConnecting);
    LOG(INFO) << "client is connecting...";

    int err;
    if((err = uv_accept(stream, (uv_stream_t*) session->GetStream())) != 0){
      LOG(ERROR) << "client accept error: " << uv_strerror(err);
      return;
    }

    LOG(INFO) << "client connected";
    if((err = uv_read_start(session->GetStream(), &ServerSession::AllocBuffer, &OnMessageReceived)) != 0){
      LOG(ERROR) << "client read error: " << uv_strerror(err);
      return;
    }
  }

  void Server::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
    ServerSession* session = (ServerSession*) stream->data;
    if(nread == UV_EOF){
      LOG(ERROR) << "client disconnected!";
      return;
    } else if(nread < 0){
      LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
      return;
    } else if(nread == 0){
      LOG(WARNING) << "zero message size received";
      return;
    } else if(nread > Session::kBufferSize){
      LOG(ERROR) << "too large of a buffer: " << nread;
      return;
    }

    BufferPtr& rbuff = session->GetReadBuffer();

    intptr_t offset = 0;
    std::vector<MessagePtr> messages;
    do{
      int32_t mtype = rbuff->GetInt();
      int64_t msize = rbuff->GetLong();

      switch(mtype){
#define DEFINE_DECODE(Name) \
                case Message::MessageType::k##Name##MessageType:{ \
                    MessagePtr msg = Name##Message::NewInstance(rbuff); \
                    LOG(INFO) << "received message: " << msg->ToString(); \
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
    session->Send(VerackMessage::NewInstance(ClientType::kNode, Server::GetID(), head->GetHeader()));

    UUID id = msg->GetID();
    if(session->IsConnecting()){
      session->SetID(id);
      session->SetState(Session::State::kConnected);
      if(msg->IsNode()){
        NodeAddress paddr = msg->GetCallbackAddress();
        LOG(INFO) << "peer connected from: " << paddr;
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
          } else if(ObjectPool::HasObject(hash)){
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
          response.push_back(TransactionMessage::NewInstance(tx));//TODO: fix
        } else if(item.IsUnclaimedTransaction()){
          if(!ObjectPool::HasUnclaimedTransaction(hash)){
            LOG(WARNING) << "couldn't find unclaimed transaction: " << hash;
            response.push_back(NotFoundMessage::NewInstance());
            break;
          }

          UnclaimedTransactionPtr utxo = ObjectPool::GetUnclaimedTransaction(hash);
          response.push_back(UnclaimedTransactionMessage::NewInstance(utxo));
        }
      } else{
        session->Send(NotFoundMessage::NewInstance());
        return;
      }
    }

    session->Send(response);
  }

  void ServerSession::HandlePrepareMessage(ServerSession* session, const PrepareMessagePtr& msg){
    ProposalPtr proposal = msg->GetProposal();
    if(!proposal){
      LOG(ERROR) << "cannot get proposal from BlockDiscoveryThread.";
      return;
    }
    proposal->SetPhase(Proposal::kProposalPhase);
  }

  void ServerSession::HandlePromiseMessage(ServerSession* session, const PromiseMessagePtr& msg){
    ProposalPtr proposal = msg->GetProposal();
    if(proposal->GetProposer() == Server::GetID()){
      proposal->AcceptProposal(session->GetID().ToString());//TODO: remove ToString()
    } else{
      proposal->SetPhase(Proposal::kVotingPhase);
    }
  }

  void ServerSession::HandleCommitMessage(ServerSession* session, const CommitMessagePtr& msg){
    ProposalPtr proposal = msg->GetProposal();
    if(proposal->GetProposer() == Server::GetID()){
      proposal->AcceptProposal(session->GetID().ToString());//TODO: remove ToString()
    } else{
      proposal->SetPhase(Proposal::kCommitPhase);
    }
  }

  void ServerSession::HandleAcceptedMessage(ServerSession* session, const AcceptedMessagePtr& msg){
    ProposalPtr proposal = msg->GetProposal();
    proposal->AcceptProposal(session->GetID().ToString()); //TODO: remove ToString()
  }

  void ServerSession::HandleRejectedMessage(ServerSession* session, const RejectedMessagePtr& msg){
    ProposalPtr proposal = msg->GetProposal();
    proposal->RejectProposal(session->GetID().ToString()); //TODO: remove ToString()
  }

  void ServerSession::HandleBlockMessage(ServerSession* session, const BlockMessagePtr& msg){
    BlockPtr blk = msg->GetValue();
    Hash hash = blk->GetHash();
    if(!ObjectPool::HasObject(hash)){
      ObjectPool::PutObject(hash, blk);
      //TODO: Server::Broadcast(InventoryMessage::NewInstance(blk));
    }
  }

  void ServerSession::HandleTransactionMessage(ServerSession* session, const TransactionMessagePtr& msg){
    TransactionPtr tx = msg->GetValue();
    Hash hash = tx->GetHash();

    LOG(INFO) << "received transaction: " << hash;
    if(!ObjectPool::HasObject(hash))
      ObjectPool::PutObject(hash, tx);
  }

  void ServerSession::HandleUnclaimedTransactionMessage(ServerSession* session, const UnclaimedTransactionMessagePtr& msg){}

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

      for(uint32_t idx = start_block->GetHeight() + 1;
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