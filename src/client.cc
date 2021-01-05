#ifdef TOKEN_ENABLE_SERVER

#include "server/server.h"
#include "vthread.h"
#include "client.h"

namespace Token{
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()
#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)

  void ClientSession::WaitForNextMessage(){
    LOCK;
    while(next_ == nullptr) WAIT;
  }

  void ClientSession::SetNextMessage(const MessagePtr& msg){
    LOCK;
    next_ = msg;
    SIGNAL_ALL;
  }

  MessagePtr ClientSession::GetNextMessage(){
    LOCK;
    MessagePtr next = next_;
    next_ = nullptr;
    return next;
  }

  void* ClientSession::SessionThread(void* data){
    ClientSession* session = (ClientSession*) data;
    uv_loop_t* loop = session->GetLoop();
    NodeAddress address = session->GetAddress();
    LOG(INFO) << "connecting to peer " << address << "....";

    uv_async_init(loop, &session->shutdown_, &OnShutdown);
    struct sockaddr_in addr;
    uv_ip4_addr(address.GetAddress().c_str(), address.GetPort(), &addr);

    uv_connect_t conn;
    conn.data = session;

    int err;
    if((err = uv_tcp_connect(&conn, session->GetHandle(), (const struct sockaddr*) &addr, &OnConnect)) != 0){
      LOG(WARNING) << "couldn't connect to peer " << address << ": " << uv_strerror(err);
      session->Disconnect();
      goto cleanup;
    }

    uv_run(loop, UV_RUN_DEFAULT);
    cleanup:
    LOG(INFO) << "disconnected from peer: " << address;
    pthread_exit(0);
  }

  void ClientSession::OnConnect(uv_connect_t* conn, int status){
    ClientSession* session = (ClientSession*) conn->data;
    if(status != 0){
      LOG(WARNING) << "client accept error: " << uv_strerror(status);
      session->Disconnect();
      return;
    }

    session->SetState(Session::kConnecting);
    session->Send(VersionMessage::NewInstance(session->GetID()));
    if((status = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
      LOG(WARNING) << "client read error: " << uv_strerror(status);
      session->Disconnect();
      return;
    }
  }

  void ClientSession::OnShutdown(uv_async_t* handle){
    ClientSession* client = (ClientSession*) handle->data;
    client->Disconnect();
  }

  void ClientSession::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
    ClientSession* client = (ClientSession*) stream->data;
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
      LOG(ERROR) << "too large of a buffer";
      return;
    }

    intptr_t offset = 0;
    BufferPtr& rbuff = client->GetReadBuffer();
    do{
      int32_t mtype = rbuff->GetInt();
      int64_t msize = rbuff->GetLong();
      switch(mtype){
        case Message::MessageType::kVersionMessageType:{
          //TODO: convert to ClientVerack or something better than this garble
          BlockPtr head = Block::Genesis();
          client->Send(VerackMessage::NewInstance(ClientType::kClient, Server::GetID(), NodeAddress(), Version(), head->GetHeader(), Hash::GenerateNonce()));
          break;
        }
        case Message::MessageType::kVerackMessageType:{
          VerackMessagePtr verack = std::make_shared<VerackMessage>(rbuff);
          client->SetState(Session::kConnected);
          LOG(INFO) << "client is connected";
          LOG(INFO) << "callback address: " << verack->GetCallbackAddress();
          break;
        }
        case Message::MessageType::kNotFoundMessageType:client->SetNextMessage(NotFoundMessage::NewInstance(rbuff));
          break;
        case Message::MessageType::kBlockMessageType:client->SetNextMessage(BlockMessage::NewInstance(rbuff));
          break;
        case Message::MessageType::kInventoryMessageType:{
          InventoryMessagePtr inv = std::make_shared<InventoryMessage>(rbuff);
          LOG(INFO) << "received inventory of size: " << inv->GetNumberOfItems();
          client->SetNextMessage(inv);
          break;
        }
        case Message::MessageType::kUnknownMessageType:
        default:LOG(ERROR) << "unknown message type " << mtype << " of size " << msize;
          break;
      }

      offset += (msize + Message::kHeaderSize);
    } while((offset + Message::kHeaderSize) < nread);
    rbuff->Reset();
  }

  bool ClientSession::SendTransaction(const TransactionPtr& tx){
    LOG(INFO) << "sending transaction " << tx->GetHash();
    if(IsConnecting()){
      LOG(INFO) << "waiting for client to connect...";
      WaitForState(Session::kConnected);
    }
    Send(TransactionMessage::NewInstance(tx));
    return true; // no acknowledgement from server
  }

  BlockPtr ClientSession::GetBlock(const Hash& hash){
    LOG(INFO) << "getting block: " << hash;
    if(IsConnecting()){
      LOG(INFO) << "waiting for the client to connect....";
      WaitForState(Session::kConnected);
    }

    std::vector<InventoryItem> items = {
      InventoryItem(InventoryItem::kBlock, hash),
    };
    Send(GetDataMessage::NewInstance(items));
    do{
      WaitForNextMessage();
      MessagePtr next = GetNextMessage();

      if(next->IsBlockMessage()){
        return std::static_pointer_cast<BlockMessage>(next)->GetValue();
      } else if(next->IsNotFoundMessage()){
        return nullptr;
      } else{
        LOG(WARNING) << "cannot handle " << next;
        continue;
      }
    } while(true);
  }

  UnclaimedTransactionPtr ClientSession::GetUnclaimedTransaction(const Hash& hash){
    return UnclaimedTransactionPtr(nullptr);
  }

  bool ClientSession::GetPeers(PeerList& peers){
    LOG(INFO) << "getting peers....";
    if(IsConnecting()){
      LOG(INFO) << "waiting for client to connect....";
      WaitForState(Session::kConnected);
    }

    Send(GetPeersMessage::NewInstance());
    do{
      WaitForNextMessage();
      MessagePtr next = GetNextMessage();
      if(next->IsPeerListMessage()){
        PeerListMessagePtr resp = std::static_pointer_cast<PeerListMessage>(next);
        std::copy(resp->peers_begin(), resp->peers_end(), std::inserter(peers, peers.begin()));
        return true;
      } else{
        //TODO: handle a better error response, this causes an infinite loop if the peer doesn't respond
        LOG(WARNING) << "cannot handle " << next;
        continue;
      }
    } while(true);
  }

  bool ClientSession::GetBlockChain(std::set<Hash>& blocks){
    LOG(INFO) << "ClientSession::GetBlockChain(std::set<Hash>&) not implemented";
    return false;
  }
}

#endif//TOKEN_ENABLE_SERVER