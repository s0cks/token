#include "old/session.h"
#include "layout.h"
#include "message_data.h"
#include "block_miner.h"
#include "old/server.h"

namespace Token {
    /*
    static const size_t kResolveWaitTimeMilliseconds = 1 * 1000;

    void ClientSession::ResolveInventory(uv_work_t* req){
        ResolveInventoryData* data = (ResolveInventoryData*)req->data;
        Session* session = data->session;

        auto& item = data->items.back();
        uint256_t hash = item.GetHash();
        if(item.IsBlock()){
            if(BlockChain::ContainsBlock(hash)){
                LOG(INFO) << hash << " found!";
                return;
            }

            LOG(INFO) << hash << " wasn't found, downloading...";
            session->SendGetData(HexString(hash));
        } else if(item.IsTransaction()){
            if(TransactionPool::HasTransaction(hash)){
                LOG(INFO) << hash << " found!";
                return;
            }
            LOG(INFO) << hash << " wasn't found in the tx pool, downloading...";
            session->SendGetData(HexString(hash));
        }
        uv_sleep(kResolveWaitTimeMilliseconds);
    }

    void ClientSession::AfterResolveInventory(uv_work_t* req, int status){
        ResolveInventoryData* data = (ResolveInventoryData*)req->data;
        Session* session = data->session;

        auto& item = data->items.back();
        uint256_t hash = item.GetHash();
        if(item.IsBlock()){
            if(!BlockChain::ContainsBlock(hash)){
                LOG(WARNING) << hash << " wasn't resolved, rescheduling";
                uv_queue_work(req->loop, req, ResolveInventory, AfterResolveInventory);
                return;
            }
        } else if(item.IsTransaction()){
            if(!TransactionPool::HasTransaction(hash)){
                LOG(WARNING) << hash << " wasn't resolved, rescheduling";
                uv_queue_work(req->loop, req, ResolveInventory, AfterResolveInventory);
                return;
            }
        }
        data->items.pop_back();
        LOG(INFO) << hash << " resolved!";

        if(!data->items.empty()){
            uv_queue_work(req->loop, req, ResolveInventory, AfterResolveInventory);
            return;
        }

        if(session->IsSynchronizing()) session->SendVerack();
    }

    void ClientSession::HandlePongMessage(uv_work_t* req){
        //TODO: implement
    }

    void ClientSession::HandleRequestVoteMessage(uv_work_t* req){
        BlockChainServer* instance = BlockChainServer::GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        RequestVoteMessage* msg = data->request->AsRequestVoteMessage();

        if(instance->IsCandidate()){
            if(instance->GetNodeID() == msg->GetSenderID()){
                LOG(INFO) << instance->GetNodeID() << " gives vote to self....";
                instance->IncrementVoteCounter();
                if(instance->GetVoteCounter() > instance->peers_.size() / 2){
                    BlockChainServer::AsyncBroadcastMessage(new ElectionMessage(instance->GetNodeID()));
                    instance->SetLeaderID(instance->GetNodeID());
                    instance->SetVoteCounter(0);
                    instance->SetLeaderTTL(0);
                    instance->SetState(BlockChainServer::kLeader);
                }
            } else if(instance->GetNodeID() != msg->GetSenderID()){
                LOG(INFO) << instance->GetNodeID() << " is in candidate state, ignoring vote request from: " << msg->GetSenderID();
            }
        } else if(instance->IsLeader()){
            //TODO:?
        } else{
            LOG(INFO) << instance->GetNodeID() << " sending vote to: " << msg->GetSenderID();
            session->SendVote(instance->GetNodeID());
        }
    }

    void ClientSession::HandleVoteMessage(uv_work_t* req){
        BlockChainServer* instance = BlockChainServer::GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        VoteMessage* msg = data->request->AsVoteMessage();

        if(instance->IsCandidate()){
            LOG(INFO) << instance->GetNodeID() << " received vote from: " << msg->GetSenderID();
            instance->IncrementVoteCounter();
            if(instance->GetVoteCounter() > instance->peers_.size() / 2){
                BlockChainServer::AsyncBroadcastMessage(new ElectionMessage(instance->GetNodeID()));
                instance->SetLeaderID(BlockChainServer::GetNodeID());
                instance->SetVoteCounter(0);
                instance->SetState(BlockChainServer::kLeader);
                instance->SetLeaderTTL(0);
                LOG(INFO) << instance->GetNodeID() << " declared itself leader";
            }
        }
    }

    void ClientSession::HandleElectionMessage(uv_work_t* req){
        BlockChainServer* instance = BlockChainServer::GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        ElectionMessage* msg = data->request->AsElectionMessage();

        LOG(INFO) << instance->GetNodeID() << " acknowledges " << msg->GetSenderID() << " as leader";
        instance->SetLeaderID(msg->GetSenderID());
        instance->SetLastHeartbeat(GetCurrentTime());
        instance->SetState(BlockChainServer::kFollower);
        instance->SetBeatCounter(0);
    }

    void ClientSession::HandleCommitMessage(uv_work_t* req){
        BlockChainServer* instance = BlockChainServer::GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        CommitMessage* msg = data->request->AsCommitMessage();

        if(BlockChainServer::IsFollower()){
            LOG(INFO) << "hb" << msg->GetSenderID();
            instance->SetLastHeartbeat(GetCurrentTime());
            instance->SetState(BlockChainServer::kFollower);
            instance->SetBeatCounter(0);
        } else if(BlockChainServer::IsLeader()){
            LOG(INFO) << "acknowledgement from follower: " << msg->GetSenderID();
        }
    }


    void ClientSession::HandleGetBlocksMessage(uv_work_t* req){
        //TODO: implement
    }

    void ClientSession::HandleInventoryMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        InventoryMessage* inventory = data->request->AsInventoryMessage();
        session->SetState(SessionState::kSynchronizing);

        std::vector<InventoryItem> items;
        if(!inventory->GetItems(items)){
            LOG(WARNING) << "couldn't get items from inventory";
            return;
        }

        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = new ResolveInventoryData(session, items);
        uv_queue_work(req->loop, work, ResolveInventory, AfterResolveInventory);
    }

    void ClientSession::HandleBlockMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        Block* block = data->request->AsBlockMessage()->GetBlock();
        LOG(INFO) << "received block: " << block->GetHash();
        if(!BlockChain::AppendBlock(block)){
            LOG(WARNING) << "couldn't schedule block for mining: " << block->GetHash();
            return;
        }
    }

    void ClientSession::HandleTransactionMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        Transaction* tx = data->request->AsTransactionMessage()->GetTransaction();
        LOG(INFO) << "received transaction: " << tx->GetHash();
        if(!TransactionPool::PutTransaction(tx)){
            LOG(WARNING) << "couldn't add transaction " << tx->GetHash() << " to pool";
            return;
        }
    }

    void ClientSession::HandleVersionMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        VersionMessage* msg = data->request->AsVersionMessage();
        Session* session = data->session;
        if(!session->IsConnecting()){
            LOG(ERROR) << "session is not handshaking";
            return;
        }
        session->SetState(SessionState::kHandshaking);
        session->SendVerack();

        //TODO: check peer count
        BlockChainServer::ConnectToPeer(msg->GetPeerAddress(), msg->GetPeerPort());
    }

    void ClientSession::HandleVerackMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        ClientSession* session = (ClientSession*)data->session;
        VerackMessage* verack = data->request->AsVerackMessage();
        if(!(session->IsHandshaking() || session->IsSynchronizing())){
            LOG(ERROR) << "session isn't handshaking";
            return;
        }

        BlockHeader head = BlockChain::GetHead();
        if(head.GetHeight() < verack->GetMaxBlock()){
            LOG(INFO) << "synchronizing " << verack->GetMaxBlock() << " blocks....";
            session->SetState(SessionState::kSynchronizing);
            session->SendGetBlocks(HexString(head.GetHash()), HexString(uint256_t()));
            return;
        }

        LOG(INFO) << "connected!";
        session->SetState(SessionState::kConnected);
    }

    void ClientSession::HandlePingMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        ClientSession* session = (ClientSession*)data->session;
        PingMessage* msg = data->request->AsPingMessage();
        LOG(INFO) << "ping: " << msg->GetNonce();
        session->SendPong(msg->GetNonce());
    }

    void ClientSession::HandleGetDataMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        ClientSession* session = (ClientSession*)data->session;
        GetDataMessage* msg = data->request->AsGetDataMessage();

        uint256_t hash = HashFromHexString(msg->GetHash());
        if(TransactionPool::HasTransaction(hash)){
            LOG(INFO) << "client requested transaction: " << hash;

            Transaction tx;
            if(!TransactionPool::GetTransaction(hash, &tx)){
                LOG(WARNING) << "cannot get transaction: " << hash;
                return;
            }

            session->SendTransaction(tx);
        } else if(BlockChain::ContainsBlock(hash)){
            LOG(INFO) << "client requested block: " << hash;

            Block blk;
            if(!BlockChain::GetBlockData(hash, &blk)){
                LOG(WARNING) << "cannot get block: " << hash;
                return;
            }

            session->SendBlock(blk);
        }
    }

    void ClientSession::AfterHandleMessage(uv_work_t* req, int status){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        if(data->request) free(data->request);
        free(req->data);
        free(req);
    }

    void ClientSession::OnMessageReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
        ClientSession *session = (ClientSession*)stream->data;
        if (nread == UV_EOF) {
            LOG(ERROR) << "disconnected from peer: " << uv_strerror(nread);
            return;
        } else if (nread > 0) {
            ByteBuffer *rbuffer = session->GetReadBuffer();
            Message::Type type = static_cast<Message::Type>(rbuffer->GetInt());
            uint64_t size = rbuffer->GetLong();

            uint8_t mbytes[size];
            rbuffer->GetBytes(mbytes, size);
            rbuffer->clear();

            Message *msg;
            if (!(msg = Message::Decode(type, mbytes, size))) {
                LOG(ERROR) << "couldn't decode message from byte array";
                return;
            }

            uv_work_t *work = (uv_work_t *) malloc(sizeof(uv_work_t));
            work->data = new ProcessMessageData(session, msg);

#define DECLARE_HANDLE(Name) \
    if(msg->Is##Name##Message()){ uv_queue_work(stream->loop, work, Handle##Name##Message, AfterHandleMessage); }

            FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLE);
        }
    }

    void ClientSession::OnConnect(uv_connect_t *conn, int status) {
        ClientSession *session = (ClientSession *) conn->data;
        if (status != 0) {
            LOG(ERROR) << "error connecting: " << uv_strerror(status);
            return;
        }
        session->SetState(SessionState::kConnecting);
        session->handle_ = conn->handle;
        session->handle_->data = session;
        uv_read_start(session->handle_, AllocBuffer, &OnMessageReceived);

        std::string address = "localhost"; //TODO: retrieve proper address
        uint32_t port = FLAGS_port;
        session->SendVersion(address, port);
    }

    void* ClientSession::ClientSessionThread(void* data){
        ClientSession* session = (ClientSession*)data;
        std::cerr << "creating client session....";
        uv_loop_t* loop = uv_loop_new();
        uv_tcp_init(loop, &session->stream_);
        struct sockaddr_in addr;
        uv_ip4_addr(session->GetAddress(), session->GetPort(), &addr);
        session->conn_.data = (void*)session;
        int err = uv_tcp_connect(&session->conn_, &session->stream_, (const struct sockaddr*)&addr, &OnConnect);
        if(err != 0){
            LOG(ERROR) << "cannot connect to peer: " << uv_strerror(err);
        }
        uv_run(loop, UV_RUN_DEFAULT);
        return nullptr;
    }

    int ClientSession::Connect() {
        LOG(INFO) << "connecting to: " << GetAddress() << ":" << GetPort();
        return pthread_create(&thread_, NULL, &ClientSessionThread, (void *) this);
    }
    */
}