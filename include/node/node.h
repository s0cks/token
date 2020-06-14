#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include "message.h"
#include "session.h"

namespace Token{
    class HandleMessageTask;
    class Node{
    public:
        enum State{
            kStarting,
            kRunning,
            kSynchronizing,
            kStopping,
            kStopped
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
                case kStarting:
                    stream << "Starting";
                    break;
                case kRunning:
                    stream << "Running";
                    break;
                case kSynchronizing:
                    stream << "Synchronizing";
                    break;
                case kStopping:
                    stream << "Stopping";
                    break;
                case kStopped:
                    stream << "Stopped";
                    break;
                default:
                    stream << "Unknown";
                    break;
            }
            return stream;
        }
    private:
        pthread_t thid_;
        pthread_rwlock_t rwlock_;

        uv_tcp_t server_;
        NodeInfo info_;

        pthread_mutex_t smutex_;
        pthread_cond_t scond_;
        State state_;

        std::map<std::string, PeerSession*> peers_;

        static void RegisterPeer(const std::string& node_id, PeerSession* peer);
        static void UnregisterPeer(const std::string& node_id);
        static bool HasPeer(const std::string& node_id);

        static void WaitForState(State state);
        static void SetState(State state);
        static State GetState();

        static inline bool
        IsStarting(){
            return GetState() == kStarting;
        }

        static inline bool
        IsRunning(){
            return GetState() == kRunning;
        }

        static inline bool
        IsSynchronizing(){
            return GetState() == kSynchronizing;
        }

        static inline bool
        IsStopping(){
            return GetState() == kStopping;
        }

        static inline bool
        IsStopped(){
            return GetState() == kStopped;
        }


        static void* NodeThread(void* ptr);
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

#define DECLARE_MESSAGE_HANDLER(Name) \
    static void Handle##Name##Message(HandleMessageTask* task);
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER);
#undef DECLARE_MESSAGE_HANDLER

#define DECLARE_TASK(Name) \
    static void Handle##Name##Task(uv_work_t* handle); \
    static void After##Name##Task(uv_work_t* handle, int status);
    DECLARE_TASK(ResolveInventory);
    DECLARE_TASK(ResolveInventoryData);
#undef DECLARE_TASK

        uv_tcp_t* GetHandle(){
            return &server_;
        }

        Node():
            thid_(),
            rwlock_(),
            state_(kStopped),
            server_(),
            smutex_(),
            scond_(),
            info_(&server_){
            server_.data = this;
            pthread_mutex_init(&smutex_, 0);
            pthread_cond_init(&scond_, 0);
        }

        friend class BlockMiner;
        friend class PeerSession; //TODO: revoke access
    public:
        ~Node(){}

        static Node* GetInstance();
        static uint32_t GetNumberOfPeers();
        static NodeInfo GetInfo();
        static bool ConnectTo(const NodeAddress& address);
        static bool Broadcast(Message* msg);


        static inline bool
        BroadcastInventory(Block* block){
            std::vector<InventoryItem> items = {
                InventoryItem(block)
            };
            return Broadcast(InventoryMessage::NewInstance(items));
        }

        static inline bool
        BroadcastInventory(Transaction* tx){
            std::vector<InventoryItem> items = {
                InventoryItem(tx)
            };
            return Broadcast(InventoryMessage::NewInstance(items));
        }

        static inline bool
        ConnectTo(const std::string& address, uint32_t port){
            return ConnectTo(NodeAddress(address, port));
        }

        static bool Start(){
            if(!IsStopped()) return false;
            Node* node = GetInstance();
            return pthread_create(&node->thid_, NULL, &NodeThread, node) == 0;
        }

        static bool WaitForShutdown(){
            if(IsStarting()) WaitForState(State::kRunning);
            if(!IsRunning()) return false;
            Node* node = GetInstance();
            LOG(INFO) << "waiting for shutdown...";
            return pthread_join(node->thid_, NULL) == 0;
        }
    };
}

#endif //TOKEN_NODE_H