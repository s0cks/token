#include <uuid/uuid.h>
#include <algorithm>
#include <random>
#include <condition_variable>

#include "server.h"
#include "message.h"
#include "task.h"
#include "configuration.h"
#include "peer.h"
#include "byte_buffer.h"

namespace Token{
    static uv_tcp_t handle_;
    static uv_async_t aterm_;

    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static Server::State state_ = Server::State::kStopped;
    static std::string node_id_;

    static std::map<NodeAddress, PeerSession*> peers_;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static inline std::string
    GenerateNewUUID(){
        uuid_t uuid;
        uuid_generate_time_safe(uuid);
        char uuid_str[37];
        uuid_unparse(uuid, uuid_str);
        return std::string(uuid_str);
    }

    void Server::LoadNodeInformation(){
        LOCK_GUARD;
        libconfig::Setting& node = BlockChainConfiguration::GetProperty("Server", libconfig::Setting::TypeGroup);
        if(!node.exists("id")){
            node_id_ = GenerateNewUUID();
            node.add("id", libconfig::Setting::TypeString) = node_id_;
            BlockChainConfiguration::SaveConfiguration();
            return;
        }

        node.lookupValue("id", node_id_);
    }

    void Server::LoadPeers(){
        LOCK_GUARD;
        // load peers from configuration
        libconfig::Setting& peers = BlockChainConfiguration::GetProperty("Peers", libconfig::Setting::TypeArray);
        auto iter = peers.begin();
        while(iter != peers.end()){
            NodeAddress address((*iter));
            if(!HasPeer(address)){
                if(!ConnectTo(address)) LOG(WARNING) << "couldn't connect to peer: " << address;
            }
            iter++;
        }

        // try to load peer from flags
        if(!FLAGS_peer_address.empty() || FLAGS_peer_port > 0){
            std::string address = !FLAGS_peer_address.empty() ?
                            FLAGS_peer_address :
                            "127.0.0.1";
            uint32_t port = FLAGS_peer_port > 0 ?
                            FLAGS_peer_port :
                            8080;
            NodeAddress paddress(address, port);
            if(!HasPeer(paddress)){
                if(!ConnectTo(paddress)) LOG(WARNING) << "couldn't connect to peer: " << paddress;
            }
        }
    }

    bool Server::GetPeers(std::vector<PeerInfo>& peers){
        for(auto& it : peers_)
            peers.push_back(it.second->GetInfo());
        return peers.size() == peers_.size();
    }

    void Server::SavePeers(){
        LOCK_GUARD;
        libconfig::Setting& root = BlockChainConfiguration::GetRoot();
        if(root.exists("Peers")) root.remove("Peers");

        libconfig::Setting& peers = root.add("Peers", libconfig::Setting::TypeArray);
        for(auto& it : peers_){
            NodeAddress address = it.second->GetAddress();
            peers.add(libconfig::Setting::TypeString) = address.ToString();
        }

        BlockChainConfiguration::SaveConfiguration();
    }

    uv_tcp_t* Server::GetHandle(){
        return &handle_;
    }

    std::string Server::GetID(){
        LOCK_GUARD;
        return node_id_;
    }

    /*
    bool Server::Shutdown(){
        if(!IsRunning()) return false;
        uv_async_send(&aterm_);

        int ret;
        if((ret = pthread_join(thread_, NULL)) != 0){
            std::stringstream ss;
            ss << "Couldn't join server thread: " << strerror(ret);
            CrashReport::GenerateAndExit(ss);
        }

        SetState(State::kStopped);
        return true;
    }
    */

    void Server::SetState(Server::State state){
        LOCK;
        state_ = state;
        SIGNAL_ALL;
    }

    void Server::WaitForState(Server::State state){
        LOCK;
        while(state_ != state) WAIT;
    }

    Server::State Server::GetState() {
        LOCK_GUARD;
        return state_;
    }

    size_t Server::GetNumberOfPeers(){
        LOCK_GUARD;
        return peers_.size();
    }

    void Server::HandleTerminateCallback(uv_async_t* handle){
        uv_stop(handle->loop);
    }

    void Server::HandleThread(uword parameter){
        LOG(INFO) << "starting server...";
        SetState(State::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &aterm_, &HandleTerminateCallback);

        struct sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", FLAGS_port, &addr);
        uv_tcp_init(loop, GetHandle());
        uv_tcp_keepalive(GetHandle(), 1, 60);
        int err;
        if((err = uv_tcp_bind(GetHandle(), (const struct sockaddr*)&addr, 0)) != 0){
            LOG(ERROR) << ""; //TODO:
            pthread_exit(0);
        }

        if((err = uv_listen((uv_stream_t*)GetHandle(), 100, &OnNewConnection)) != 0){
            LOG(ERROR) << ""; //TODO:
            pthread_exit(0);
        }

#ifdef TOKEN_DEBUG
        LOG(INFO) << "server " << GetID() << " listening @" << FLAGS_port;
#else
        LOG(INFO) << "server listening @" << FLAGS_port;
#endif//TOKEN_DEBUG
        SetState(State::kRunning);
        uv_run(loop, UV_RUN_DEFAULT);

        uv_loop_close(loop);
        pthread_exit(0);
    }

    void Server::OnNewConnection(uv_stream_t* stream, int status){
        if(status != 0){
            LOG(ERROR) << "connection error: " << uv_strerror(status);
            return;
        }

        NodeSession* session = new NodeSession();
        uv_tcp_init(stream->loop, (uv_tcp_t*)session->GetStream());
        LOG(INFO) << "client is connecting...";
        if((status = uv_accept(stream, (uv_stream_t*)session->GetStream())) != 0){
            LOG(ERROR) << "client accept error: " << uv_strerror(status);
            return;
        }

        LOG(INFO) << "client connected";
        if((status = uv_read_start(session->GetStream(), &Session::AllocBuffer, &OnMessageReceived)) != 0){
            LOG(ERROR) << "client read error: " << uv_strerror(status);
            return;
        }
    }

    void Server::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        NodeSession* session = (NodeSession*)stream->data;
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

        std::vector<Handle<Message>> messages;
        do{
            ByteBuffer bytes((uint8_t*)buff->base, buff->len);
            uint32_t mtype = bytes.GetInt();
            uint64_t msize = bytes.GetLong();
            Handle<Message> msg = Message::Decode(static_cast<Message::MessageType>(mtype), &bytes);
            LOG(INFO) << "decoded message: " << msg->ToString(); //TODO: handle decode failures
            messages.push_back(msg);

            offset += (msize + Message::kHeaderSize);
        } while((offset + Message::kHeaderSize) < nread);

        for(size_t idx = 0; idx < messages.size(); idx++){
            Handle<Message> msg = messages[idx];
            Handle<HandleMessageTask> task = HandleMessageTask::NewInstance(session, msg);
            switch(msg->GetMessageType()){
#define DEFINE_HANDLER_CASE(Name) \
            case Message::k##Name##MessageType: \
                session->Handle##Name##Message(task); \
                break;
            FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE);
#undef DEFINE_HANDLER_CASE
                case Message::kUnknownMessageType:
                default: //TODO: handle properly
                    break;
            }
        }
    }

    bool Server::Broadcast(const Handle<Message>& msg){
        LOCK_GUARD;
        for(auto& it : peers_) it.second->Send(msg);
        return true;
    }

    bool Server::HasPeer(const std::string& node_id){
        LOCK_GUARD;
        for(auto& it : peers_){
            PeerSession* peer = it.second;
            if(peer->GetID() == node_id) return true;
        }
        return false;
    }

    bool Server::HasPeer(const NodeAddress& address){
        LOCK_GUARD;
        return peers_.find(address) != peers_.end();
    }

    bool Server::ConnectTo(const NodeAddress &address){
        //-- Attention! --
        // this is not a memory leak, the memory will be freed upon
        // the session being closed
        return (new PeerSession(address))->Connect();
    }

    void Server::RegisterPeer(PeerSession* peer){
        LOCK_GUARD;
        NodeAddress paddr = peer->GetAddress();
        auto pos = peers_.find(paddr);
        if(pos != peers_.end()) return;
        peers_.insert({ paddr, peer });
    }

    void Server::UnregisterPeer(PeerSession* peer){
        LOCK_GUARD;
        NodeAddress paddr = peer->GetAddress();
        auto pos = peers_.find(paddr);
        if(pos == peers_.end()) return;
        peers_.insert({ paddr, peer });
    }
}