#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include "message.h"
#include "buffer.h"

namespace Token{
    class Session{
    protected:
        pthread_mutex_t rmutex_;
        pthread_cond_t rcond_;
        ByteBuffer rbuffer_;
        ByteBuffer wbuffer_;

        Session():
            rmutex_(PTHREAD_MUTEX_INITIALIZER),
            rcond_(),
            rbuffer_(4096),
            wbuffer_(4096){
            pthread_mutexattr_t rmutex_attr;
            pthread_mutexattr_init(&rmutex_attr);
            pthread_mutexattr_settype(&rmutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
            pthread_mutex_init(&rmutex_, &rmutex_attr);
        }

        static void OnMessageSent(uv_write_t *req, int status);
        virtual uv_stream_t* GetStream() = 0;

        bool IsResolved(const InventoryItem& item){
            uint256_t hash = item.GetHash();
            if(item.IsTransaction()){
                Transaction* tx;
                if((tx = TransactionPool::GetTransaction(hash))){
                    return true;
                }
            } else if(item.IsBlock()){
                Block* blk;
                if((blk = BlockChain::GetBlockData(hash))){
                    return true;
                }

                if((blk = BlockPool::GetBlock(hash))){
                    return true;
                }
            }
            return false;
        }

        inline void
        WaitForTransaction(const uint256_t& hash){
            WaitForItem(InventoryItem(InventoryItem::kTransaction, hash));
        }

        inline void
        WaitForBlock(const uint256_t& hash){
            WaitForItem(InventoryItem(InventoryItem::kBlock, hash));
        }

        void WaitForItem(const InventoryItem& item){
            pthread_mutex_trylock(&rmutex_);
            LOG(INFO) << "waiting for: " << item.GetHash();
            while(!IsResolved(item)) pthread_cond_wait(&rcond_, &rmutex_);
            pthread_mutex_unlock(&rmutex_);
        }

        void WaitForItems(std::vector<InventoryItem> items){
            if(items.empty()) return;
            InventoryItem next = items.back();
            items.pop_back();
            WaitForItem(next);
            return WaitForItems(items);
        }

        void OnHash(const uint256_t& hash){
            pthread_mutex_trylock(&rmutex_);
            pthread_cond_signal(&rcond_);
            pthread_mutex_unlock(&rmutex_);
        }
    public:
        virtual ~Session() = default;

        ByteBuffer* GetReadBuffer(){
            return &rbuffer_;
        }

        ByteBuffer* GetWriteBuffer(){
            return &wbuffer_;
        }

        void Send(Message* msg);
        void Send(std::vector<Message*>& messages);
    };

    class NodeSession : public Session{
    public:
        enum State{
            kConnecting,
            kConnected,
            kDisconnected,
        };

        friend class Node;
    private:
        pthread_rwlock_t rwlock_;
        State state_;
        NodeInfo info_;
        uv_tcp_t handle_;
        pthread_mutex_t rmutex_;
        pthread_cond_t rcond_;

        NodeSession():
                state_(kDisconnected),
                handle_(),
                info_(&handle_),
                rmutex_(),
                rcond_(){
            pthread_mutex_init(&rmutex_, NULL);
            pthread_cond_init(&rcond_, NULL);

            handle_.data = this;
            SetState(kConnecting);
        }

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&handle_;
        }
    public:
        ~NodeSession(){}

        NodeInfo GetInfo() const{
            return info_;
        }

        std::string GetNodeID() const{
            return GetInfo().GetNodeID();
        }

        NodeAddress GetAddress() const{
            return GetInfo().GetNodeAddress();
        }

        State GetState(){
            pthread_rwlock_rdlock(&rwlock_);
            State state = state_;
            pthread_rwlock_unlock(&rwlock_);
            return state;
        }

        bool IsConnecting(){
            return GetState() == kConnecting;
        }

        bool IsConnected(){
            return GetState() == kConnected;
        }

        bool IsDisconnected(){
            return GetState() == kDisconnected;
        }

        void SetState(State state){
            pthread_rwlock_trywrlock(&rwlock_);
            state_ = state;
            pthread_rwlock_unlock(&rwlock_);
        }
    };

    class HandleMessageTask;
    class PeerSession : public Session{
    private:
        pthread_t thread_;
        uv_connect_t conn_;
        uv_tcp_t socket_;
        uv_async_t send_cb_;
        std::string node_id_;
        NodeAddress node_addr_;

        static void* PeerSessionThread(void* data);
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

#define DECLARE_MESSAGE_HANDLER(Name) \
    static void Handle##Name##Message(HandleMessageTask* task);
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER);

        static void HandleSynchronizeBlocksTask(uv_work_t* handle);
        static void AfterSynchronizeBlocksTask(uv_work_t* handle, int status);
    protected:
        virtual uv_stream_t* GetStream(){
            return conn_.handle;
        }

        uv_loop_t* GetLoop() const{
            return socket_.loop;
        }
    public:
        PeerSession(const NodeAddress& addr):
                Session(),
                thread_(),
                socket_(),
                conn_(),
                node_addr_(addr),
                node_id_(){
            socket_.data = this;
            conn_.data = this;
        }
        ~PeerSession(){}

        NodeInfo GetInfo() const{
            return NodeInfo(node_id_, node_addr_);
        }

        NodeAddress GetAddress() const{
            return node_addr_;
        }

        std::string GetID() const{
            return node_id_;
        }

        bool Connect(){
            return pthread_create(&thread_, NULL, &PeerSessionThread, (void*)this) == 0;
        }
    };

    //TODO: rename ClientSession
    class NodeClient : public Session{
    public:
        enum State{
            kStarting,
            kRunning,
            kStopping,
            kStopped,
        };
    private:
        uv_loop_t* loop_;
        uv_signal_t sigterm_;
        uv_signal_t sigint_;
        uv_tcp_t stream_;
        uv_connect_t connection_;
        uv_pipe_t stdin_;
        uv_pipe_t stdout_;

        State state_;
        pthread_mutex_t smutex_;
        pthread_cond_t scond_;

        void SetState(State state);
        void WaitForState(State state);
        State GetState();

        inline bool
        IsStarting(){
            return GetState() == kStarting;
        }

        inline bool
        IsRunning(){
            return GetState() == kRunning;
        }

        inline bool
        IsStopping(){
            return GetState() == kStopping;
        }

        inline bool
        IsStopped(){
            return GetState() == kStopped;
        }

        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnCommandReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnSignal(uv_signal_t* handle, int signum);

#define DECLARE_MESSAGE_HANDLER(Name) \
    static void Handle##Name##Message(HandleMessageTask* task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER);
#undef DECLARE_MESSAGE_HANDLER

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&stream_;
        }

        inline uv_stream_t*
        GetStdinPipe(){
            return (uv_stream_t*)&stdin_;
        }
    public:
        NodeClient():
                loop_(uv_loop_new()),
                state_(State::kStopped),
                sigterm_(),
                sigint_(),
                stream_(),
                stdin_(),
                stdout_(){
            stdin_.data = this;
            stdout_.data = this;
            stream_.data = this;
            connection_.data = this;
        }
        ~NodeClient(){}

        void Connect(const NodeAddress& addr);
        bool WaitForShutdown();
    };
}

#endif //TOKEN_SESSION_H