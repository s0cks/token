#include <glog/logging.h>
#include <string.h>
#include <random>

#include "block_miner.h"
#include "node/server.h"
#include "layout.h"
#include "message_data.h"

namespace Token{
    /*
    static uv_loop_t* loop = nullptr;
    static uv_tcp_t server;
    static uv_timer_t hb_timer_;
    static uv_async_t broadcast_handle_;

    void BlockChainServer::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        BlockChainServer* instance = GetInstance();
        buff->base = (char*)malloc(suggested_size);
        buff->len = suggested_size;
    }

    BlockChainServer::BlockChainServer():
        sessions_(),
        rwlock_(),
        peers_(),
        leader_id_(),
        node_id_(HashFromHexString(GenerateNonce())),
        state_(kFollower),
        leader_ttl_(0),
        beat_counter_(0),
        thread_(){}

    void BlockChainServer::SetVoteCounter(int count){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_trywrlock(&instance->rwlock_);
        instance->vote_counter_ = count;
        pthread_rwlock_unlock(&instance->rwlock_);
    }

    void BlockChainServer::SetBeatCounter(int count){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_trywrlock(&instance->rwlock_);
        instance->beat_counter_ = count;
        pthread_rwlock_unlock(&instance->rwlock_);
    }

    void BlockChainServer::SetLeaderTTL(int ttl){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_trywrlock(&instance->rwlock_);
        instance->leader_ttl_ = ttl;
        pthread_rwlock_unlock(&instance->rwlock_);
    }

    void BlockChainServer::SetLastHeartbeat(uint32_t ts){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_trywrlock(&instance->rwlock_);
        instance->last_hb_ = ts;
        pthread_rwlock_unlock(&instance->rwlock_);
    }

    void BlockChainServer::IncrementVoteCounter(){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_trywrlock(&instance->rwlock_);
        instance->vote_counter_++;
        pthread_rwlock_unlock(&instance->rwlock_);
    }

    void BlockChainServer::IncrementBeatCounter(){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_trywrlock(&instance->rwlock_);
        instance->beat_counter_++;
        pthread_rwlock_unlock(&instance->rwlock_);
    }

    void BlockChainServer::IncrementLeaderTTL(){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_trywrlock(&instance->rwlock_);
        instance->leader_ttl_++;
        pthread_rwlock_unlock(&instance->rwlock_);
    }

    void BlockChainServer::SetLeaderID(const Token::uint256_t& id){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_trywrlock(&instance->rwlock_);
        instance->leader_id_ = id;
        pthread_rwlock_unlock(&instance->rwlock_);
    }

    int BlockChainServer::GetVoteCounter(){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_tryrdlock(&instance->rwlock_);
        int count = instance->vote_counter_;
        pthread_rwlock_unlock(&instance->rwlock_);
        return count;
    }

    int BlockChainServer::GetBeatCounter(){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_tryrdlock(&instance->rwlock_);
        int count = instance->beat_counter_;
        pthread_rwlock_unlock(&instance->rwlock_);
        return count;
    }

    int BlockChainServer::GetLeaderTTL(){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_tryrdlock(&instance->rwlock_);
        int count = instance->leader_ttl_;
        pthread_rwlock_unlock(&instance->rwlock_);
        return count;
    }

    uint256_t BlockChainServer::GetLeaderID(){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_tryrdlock(&instance->rwlock_);
        uint256_t leader = instance->leader_id_;
        pthread_rwlock_unlock(&instance->rwlock_);
        return leader;
    }

    uint32_t BlockChainServer::GetLastHeartbeat(){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_tryrdlock(&instance->rwlock_);
        uint32_t ts = instance->last_hb_;
        pthread_rwlock_unlock(&instance->rwlock_);
        return ts;
    }

    void BlockChainServer::SetState(Token::BlockChainServer::ClusterMemberState state){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_trywrlock(&instance->rwlock_);
        instance->state_ = state;
        pthread_rwlock_unlock(&instance->rwlock_);
    }

    BlockChainServer::ClusterMemberState BlockChainServer::GetState(){
        BlockChainServer* instance = GetInstance();
        pthread_rwlock_tryrdlock(&instance->rwlock_);
        ClusterMemberState state = instance->state_;
        pthread_rwlock_unlock(&instance->rwlock_);
        return state;
    }

    BlockChainServer* BlockChainServer::GetInstance(){
        static BlockChainServer instance;
        return &instance;
    }

    void BlockChainServer::OnNewConnection(uv_stream_t* stream, int status){
        BlockChainServer* instance = GetInstance();
        LOG(INFO) << "client connecting....";
        if(status != 0){
            LOG(ERROR) << "connection error: " << std::string(uv_strerror(status));
            return;
        }

        PeerSession* session = new PeerSession();
        instance->sessions_.insert(std::make_pair((uv_stream_t*)session->GetHandle(), session));
        uv_tcp_init(loop, session->GetHandle());
        uv_timer_init(loop, &session->ping_timer_);
        uv_timer_init(loop, &session->timeout_timer_);
        if((status = uv_accept(stream, (uv_stream_t*)session->GetHandle())) != 0){
            LOG(ERROR) << "error accepting connection: " << uv_strerror(status);
            return;
        }
        session->SetState(SessionState::kConnecting);
        LOG(INFO) << "client accepted!";
        uv_read_start(
                (uv_stream_t*)session->GetStream(),
                &AllocBuffer,
                &OnMessageReceived
        );
    }

    void BlockChainServer::HandleVersionMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        VersionMessage* vers = data->request->AsVersionMessage();
        Session* session = data->session;
        if(!session->IsConnecting()){
            LOG(INFO) << "session not connecting";
            return;
        }
        session->SetState(SessionState::kHandshaking);

        BlockChainServer::ConnectToPeer(vers->GetPeerAddress(), vers->GetPeerPort());

        std::string address = "localhost"; //TODO: retrieve proper address
        uint32_t port = FLAGS_port;
        session->SendVersion(address, port);
    }

    void BlockChainServer::HandleVerackMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        PeerSession* session = (PeerSession*)data->session;
        VerackMessage* verack = data->request->AsVerackMessage();
        session->SendVerack();
        if(session->IsSynchronizing()){
            return;
        } else if(session->IsHandshaking()){
            session->SetState(SessionState::kConnected);
            if(verack->GetMaxBlock() < BlockChain::GetHeight()) session->SetState(SessionState::kSynchronizing);
            return;
        }
        LOG(ERROR) << "session is not handshaking or synchronizing";
        return;
    }

    void BlockChainServer::HandleGetDataMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        GetDataMessage* request = data->request->AsGetDataMessage();

        uint256_t hash = HashFromHexString(request->GetHash());
        Block block;
        if(!BlockChain::GetBlockData(hash, &block)){
            LOG(ERROR) << "couldn't find block: " << hash;
            return;
        }
        session->SendBlock(block);
    }

    void BlockChainServer::HandleGetBlocksMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        GetBlocksMessage* request = data->request->AsGetBlocksMessage();

        Proto::BlockChainServer::Inventory inventory;

        uint256_t firstHash = request->GetFirst();
        uint256_t lastHash = request->GetLast();

        LOG(INFO) << "getting " << firstHash << ":" << lastHash;

        BlockHeader firstHeader = BlockChain::GetBlock(firstHash);
        BlockHeader lastHeader;
        if(lastHash.IsNull()){
            lastHeader = BlockChain::GetHead();
        } else{
            lastHeader = BlockChain::GetBlock(lastHash);
        }

        BlockHeader current = lastHeader;
        while(!current.GetPreviousHash().IsNull()){
            uint256_t hash = current.GetHash();
            uint256_t next = current.GetPreviousHash();

            InventoryItem::RawType* item = inventory.add_items();
            item->set_type(static_cast<uint32_t>(InventoryItem::Type::kBlock));
            item->set_hash(HexString(hash));
        }

        InventoryMessage msg(inventory);
        session->Send(&msg);
    }

    void BlockChainServer::HandleBlockMessage(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Block* block = data->request->AsBlockMessage()->GetBlock();
        if(!BlockChain::AppendBlock(block)){
            LOG(ERROR) << "couldn't append block: " << block->GetHash();
            return;
        }
    }

    void BlockChainServer::HandleTransactionMessage(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Transaction* tx = data->request->AsTransactionMessage()->GetTransaction();
        if(!TransactionPool::PutTransaction(tx)){
            LOG(ERROR) << "couldn't add transaction to pool: " << tx->GetHash();
            return;
        }
    }

    void BlockChainServer::HandlePingMessage(uv_work_t* req){
        //fallthrough
    }

    void BlockChainServer::HandlePongMessage(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        PeerSession* session = (PeerSession*)data->session;
        PongMessage* msg = data->request->AsPongMessage();
        LOG(INFO) << "pong: " << msg->GetNonce();
        uv_timer_stop(&session->timeout_timer_);
    }

    void BlockChainServer::HandleResolveInventory(uv_work_t* req){
        ResolveInventoryData* data = (ResolveInventoryData*)req->data;
        Session* session = data->session;
        for(auto& item : data->items){
            uint256_t hash = item.GetHash();
            if(item.IsTransaction() && TransactionPool::HasTransaction(hash)){
                LOG(INFO) << hash << " found!";
                return;
            } else if(item.IsBlock() && BlockChain::ContainsBlock(hash)){
                LOG(INFO) << hash << " found!";
                return;
            }

            LOG(INFO) << "couldn't find: " << hash;
            session->SendGetData(HexString(hash));
        }
    }

    void BlockChainServer::HandleInventoryMessage(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        InventoryMessage* msg = data->request->AsInventoryMessage();

        std::vector<InventoryItem> items;
        msg->GetItems(items);

        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = new ResolveInventoryData(data->session, items);
        uv_queue_work(req->loop, work, HandleResolveInventory, AfterResolveInventory);
    }

    void BlockChainServer::AfterResolveInventory(uv_work_t* req, int status){
        free(req->data);
        free(req);
    }

    void BlockChainServer::AfterHandleMessage(uv_work_t* req, int status){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        if(data->request) free(data->request);
        free(req->data);
        free(req);
    }

    void BlockChainServer::OnMessageReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t* buff){
        BlockChainServer* instance = GetInstance();
        auto pos = instance->sessions_.find(stream);
        if(pos == instance->sessions_.end()){
            LOG(ERROR) << "couldn't find session";
            return;
        }

        Session* session = pos->second;
        if(nread == UV_EOF){
            LOG(ERROR) << "client disconnected!";
            return;
        } else if(nread < 0){
            LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
            return;
        } else if(nread == 0){
            LOG(WARNING) << "zero message size received";
            return;
        }

        if(nread >= 4096){
            LOG(ERROR) << "too large of a buffer";
            return;
        }

        ByteBuffer bb((uint8_t*)buff->base, nread);
        Message* msg;
        if(!(msg = Message::Decode(&bb))){
            LOG(INFO) << "couldn't decode message!";
            return;
        }

        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = new ProcessMessageData(session, msg);
#define DEFINE_HANDLER_CASE(Name) \
    if(msg->Is##Name##Message()){ uv_queue_work(stream->loop, work, Handle##Name##Message, AfterHandleMessage); }
        FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE);
    }

    void BlockChainServer::ConnectToPeer(const std::string &address, uint16_t port){
        if(!IsConnectedTo(address, port)){
            ClientSession* session = new ClientSession(address, port);
            if(session->Connect() != 0){
                LOG(ERROR) << "couldn't connect to peer";
            }
            std::stringstream key;
            key << address << ":" << port;
            GetInstance()->peers_.insert({ key.str(), session });
        }
    }

    void BlockChainServer::HandleRequestVoteMessage(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        RequestVoteMessage* msg = data->request->AsRequestVoteMessage();

        if(IsCandidate()){
            if(GetNodeID() == msg->GetSenderID()){
                LOG(INFO) << GetNodeID() << " gives vote to self....";
                instance->IncrementVoteCounter();
                if(instance->GetVoteCounter() > instance->peers_.size() / 2){
                    BroadcastMessage(ElectionMessage(GetNodeID()));
                    instance->SetLeaderID(GetNodeID());
                    instance->SetVoteCounter(0);
                    instance->SetLeaderTTL(0);
                    instance->SetState(kLeader);
                }
            } else if(GetNodeID() != msg->GetSenderID()){
                LOG(INFO) << GetNodeID() << " is in candidate state, ignoring vote request from: " << msg->GetSenderID();
            }
        } else if(IsLeader()){
            //TODO:?
        } else{
            LOG(INFO) << GetNodeID() << " sending vote to: " << msg->GetSenderID();
            session->SendVote(GetNodeID());
        }

        BroadcastMessage(new RequestVoteMessage(msg->GetSenderID()));
    }

    void BlockChainServer::HandleVoteMessage(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        VoteMessage* msg = data->request->AsVoteMessage();

        if(IsCandidate()){
            LOG(INFO) << GetNodeID() << " received vote from: " << msg->GetSenderID();
            instance->IncrementVoteCounter();
            if(instance->GetVoteCounter() > instance->peers_.size() / 2){
                BroadcastMessage(ElectionMessage(GetNodeID()));
                instance->SetLeaderID(GetNodeID());
                instance->SetVoteCounter(0);
                instance->SetState(kLeader);
                instance->SetLeaderTTL(0);
                LOG(INFO) << GetNodeID() << " declared itself leader";
            }
        }
    }

    void BlockChainServer::HandleElectionMessage(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        ElectionMessage* msg = data->request->AsElectionMessage();

        LOG(INFO) << GetNodeID() << " acknowledges " << msg->GetSenderID() << " as leader";
        instance->SetLeaderID(msg->GetSenderID());
        instance->SetLastHeartbeat(GetCurrentTime());
        instance->SetState(kFollower);
        instance->SetBeatCounter(0);
    }

    void BlockChainServer::HandleCommitMessage(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        CommitMessage* msg = data->request->AsCommitMessage();

        if(IsFollower()){
            LOG(INFO) << "hb" << msg->GetSenderID();
            instance->SetLastHeartbeat(GetCurrentTime());
            instance->SetState(kFollower);
            instance->SetBeatCounter(0);
        } else if(IsLeader()){
            LOG(INFO) << "acknowledgement from follower: " << msg->GetSenderID();
        }
    }

    void BlockChainServer::OnHeartbeat(uv_timer_t* handle){
        BlockChainServer* instance = GetInstance();
        if(IsLeader()){
            instance->SetBeatCounter(0);
            if(instance->GetLeaderTTL() <= 3){
                instance->BroadcastMessage(CommitMessage(GetNodeID()));
                instance->IncrementLeaderTTL();
            }
            return;
        } else if(IsCandidate()){
            instance->SetBeatCounter(0);
            return;
        }

        uint32_t now = GetCurrentTime();
        uint32_t delta = (now - instance->GetLastHeartbeat());
        if(delta > kBeatDelta && !IsCandidate()){
            instance->IncrementBeatCounter();
            if(instance->GetBeatCounter() > kBeatSensitivity){
                LOG(INFO) << GetNodeID() << " starting an election";
                instance->SetState(kCandidate);
                instance->IncrementVoteCounter();
                instance->BroadcastMessage(RequestVoteMessage(GetNodeID()));
                if(instance->GetVoteCounter() > instance->peers_.size() / 2){
                    instance->BroadcastMessage(ElectionMessage(GetNodeID()));
                    instance->SetLeaderID(GetNodeID());
                    instance->SetVoteCounter(0);
                    instance->SetState(kLeader);
                }
            }
        }
    }

    void BlockChainServer::HandleAsyncBroadcastMessage(uv_async_t* handle){
        Message* msg = (Message*)handle->data;
        for(auto& it : GetInstance()->peers_){
            it.second->Send(msg);
        }
        delete msg;
    }

    void* BlockChainServer::ServerThread(void* data){
        BlockChainServer* instance = GetInstance();

        loop = uv_loop_new();
        uv_timer_init(loop, &hb_timer_);
        uv_timer_start(&hb_timer_, &OnHeartbeat, kHeartbeatInit, kHeartbeatFrequency);
        uv_async_init(loop, &broadcast_handle_, HandleAsyncBroadcastMessage);

        sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", FLAGS_port, &addr);
        uv_tcp_init(loop, &server);
        uv_tcp_keepalive(&server, 1, 60);
        uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
        int err;
        if((err = uv_listen((uv_stream_t*)&server, 100, &OnNewConnection)) != 0){
            LOG(ERROR) << "couldn't start server: " << uv_strerror(err);
            pthread_exit(nullptr);
        }
        LOG(INFO) << "server listening @ localhost:" << FLAGS_port;
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(nullptr);
    }

    bool BlockChainServer::StartServer(){
        BlockChainServer* instance = GetInstance();
        return pthread_create(&instance->thread_, NULL, &ServerThread, (void*)instance) == 0;
    }

    void BlockChainServer::WaitForShutdown(){
        BlockChainServer* instance = GetInstance();
        void* result = nullptr;
        pthread_join(instance->thread_, &result);
    }

    void BlockChainServer::HandleBroadcastMessage(uv_work_t* req){
        Message* msg = (Message*)req->data;

        LOG(INFO) << "broadcasting " << msg->GetName() << " to " << GetInstance()->peers_.size() << " peers...";
        for(auto& it : GetInstance()->peers_){
            it.second->Send(msg);
        }
    }

    void BlockChainServer::AfterBroadcastMessage(uv_work_t* req, int status){
        free(req->data);
        free(req);
    }

    void BlockChainServer::AsyncBroadcastMessage(Message* msg){
        broadcast_handle_.data = msg;
        uv_async_send(&broadcast_handle_);
    }

    void BlockChainServer::BroadcastMessage(Message* msg){
        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = msg;
        uv_queue_work(loop, work, HandleBroadcastMessage, AfterBroadcastMessage);
    }
     */

    Server* Server::GetInstance(){
        static Server instance;
        return &instance;
    }

    void* Server::ServerThread(void* data){
        uv_loop_t* loop = uv_loop_new();

        struct sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", FLAGS_port, &addr);

        uv_tcp_t server;
        uv_tcp_init(loop, &server);
        uv_tcp_keepalive(&server, 1, 60);
        int err;
        if((err = uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0)) != 0){
            LOG(ERROR) << ""; //TODO:
            pthread_exit(0);
        }

        if((err = uv_listen((uv_stream_t*)&server, 100, &OnNewConnection)) != 0){
            LOG(ERROR) << ""; //TODO:
            pthread_exit(0);
        }

        LOG(INFO) << "server listening @" << FLAGS_port;
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(0);
    }

    void Server::OnNewConnection(uv_stream_t* stream, int status){

    }

}