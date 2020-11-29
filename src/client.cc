#include "server.h"
#include "task.h"
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

    void ClientSession::SetNextMessage(const Handle<Message>& msg){
        LOCK;
        WriteBarrier(&next_, msg);
        SIGNAL_ALL;
    }

    Handle<Message> ClientSession::GetNextMessage(){
        LOCK;
        Message* next = next_;
        WriteBarrier(&next_, (Message*)nullptr);
        return next;
    }

    void* ClientSession::SessionThread(void* data){
        Handle<ClientSession> session = (ClientSession*)data;
        uv_loop_t* loop = session->GetLoop();
        NodeAddress address = session->GetAddress();
        LOG(INFO) << "connecting to peer " << address << "....";

        uv_async_init(loop, &session->shutdown_, &OnShutdown);
        struct sockaddr_in addr;
        uv_ip4_addr(address.GetAddress().c_str(), address.GetPort(), &addr);

        uv_connect_t conn;
        conn.data = session;

        int err;
        if((err = uv_tcp_connect(&conn, session->GetHandle(), (const struct sockaddr*)&addr, &OnConnect)) != 0){
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
        Handle<ClientSession> session = (ClientSession*)conn->data;
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
        Handle<ClientSession> client = (ClientSession*)handle->data;
        client->Disconnect();
    }

    void ClientSession::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        Handle<ClientSession> client = (ClientSession*)stream->data;
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
        Handle<Buffer> rbuff = client->GetReadBuffer();
        do{
            int32_t mtype = rbuff->GetInt();
            int64_t msize = rbuff->GetLong();
            switch(mtype) {
                case Message::MessageType::kVersionMessageType:
                    client->Send(VerackMessage::NewInstance(client->GetID()));
                    break;
                case Message::MessageType::kVerackMessageType:
                    client->SetState(Session::kConnected);
                    LOG(INFO) << "client is connected";
                    break;
                case Message::MessageType::kNotFoundMessageType:
                    client->SetNextMessage(NotFoundMessage::NewInstance(rbuff).CastTo<Message>());
                    break;
                case Message::MessageType::kBlockMessageType:
                    client->SetNextMessage(BlockMessage::NewInstance(rbuff).CastTo<Message>());
                    break;
                case Message::MessageType::kUnclaimedTransactionMessageType:
                    client->SetNextMessage(UnclaimedTransactionMessage::NewInstance(rbuff).CastTo<Message>());
                    break;
                case Message::MessageType::kInventoryMessageType:{
                    Handle<InventoryMessage> inv = InventoryMessage::NewInstance(rbuff);
                    LOG(INFO) << "received inventory of size: " << inv->GetNumberOfItems();
                    client->SetNextMessage(inv.CastTo<Message>());
                    break;
                }
                case Message::MessageType::kUnknownMessageType:
                default:
                    LOG(ERROR) << "unknown message type " << mtype << " of size " << msize;
                    break;
            }

            offset += (msize + Message::kHeaderSize);
        } while((offset + Message::kHeaderSize) < nread);
        rbuff->Reset();
    }

    bool ClientSession::SendTransaction(const Handle<Transaction>& tx){
        LOG(INFO) << "sending transaction " << tx->GetHash();
        if(IsConnecting()){
            LOG(INFO) << "waiting for client to connect...";
            WaitForState(Session::kConnected);
        }
        Send(TransactionMessage::NewInstance(tx));
        return true; // no acknowledgement from server
    }

    Handle<Block> ClientSession::GetBlock(const Hash& hash){
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
            Handle<Message> next = GetNextMessage();

            if(next->IsBlockMessage()){
                return next.CastTo<BlockMessage>()->GetBlock();
            } else if(next->IsNotFoundMessage()){
                return Handle<Block>();
            } else{
                LOG(WARNING) << "cannot handle " << next;
                continue;
            }
        } while(true);
    }

    Handle<UnclaimedTransaction> ClientSession::GetUnclaimedTransaction(const Hash& hash){
        LOG(INFO) << "getting unclaimed transactions " << hash;
        if(IsConnecting()){
            LOG(INFO) << "waiting for client to connect...";
            WaitForState(Session::kConnected);
        }

        std::vector<InventoryItem> items = {
            InventoryItem(InventoryItem::kUnclaimedTransaction, hash),
        };
        Send(GetDataMessage::NewInstance(items));
        do{
            WaitForNextMessage();
            Handle<Message> next = GetNextMessage();

            if(next->IsUnclaimedTransactionMessage()){
                return next.CastTo<UnclaimedTransactionMessage>()->GetUnclaimedTransaction();
            } else if(next->IsNotFoundMessage()){
                return Handle<UnclaimedTransaction>();
            } else{
                LOG(WARNING) << "cannot handle " << next;
                continue;
            }
        } while(true);
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
            Handle<Message> next = GetNextMessage();
            if(next->IsPeerListMessage()){
                Handle<PeerListMessage> resp = next.CastTo<PeerListMessage>();
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

    bool ClientSession::GetUnclaimedTransactions(const User& user, std::vector<Hash>& utxos){
        LOG(INFO) << "getting unclaimed transactions for " << user;
        if(!IsConnected()){
            LOG(INFO) << "waiting for client to connect...";
            WaitForState(Session::kConnected);
        }

        Send(GetUnclaimedTransactionsMessage::NewInstance(user));
        do{
            WaitForNextMessage();
            Handle<Message> next = GetNextMessage();
            LOG(INFO) << "next: " << next->GetName();
            if(next->IsInventoryMessage()){
                Handle<InventoryMessage> inv = next.CastTo<InventoryMessage>();

                std::vector<InventoryItem> items;
                if(!inv->GetItems(items)){
                    LOG(ERROR) << "cannot get items from inventory";
                    return false;
                }

                LOG(INFO) << "received inventory of " << items.size() << " items";
                for(auto& item : items){
                    LOG(INFO) << "- " << item;
                    utxos.push_back(item.GetHash());
                }

                return utxos.size() == items.size();
            } else if(next->IsNotFoundMessage()){
                return false;
            } else{
                LOG(WARNING) << "cannot handle " << next;
                continue;
            }
        } while(true);
    }
}