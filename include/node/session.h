#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include "message.h"

namespace Token{
    class Session{
    public:
        virtual ~Session() = default;

        virtual void Send(const Message& msg) = 0;
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
        uv_tcp_t handle_;
        NodeInfo info_;

        pthread_mutex_t rmutex_;
        pthread_cond_t rcond_;
        pthread_mutex_t smutex_;

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

        uv_stream_t* GetHandle(){
            return (uv_stream_t*)&handle_;
        }

        bool IsResolved(const InventoryItem& item){
            uint256_t hash = item.GetHash();
            if(item.IsTransaction()){
                Transaction* tx;
                if((tx = TransactionPool::GetTransaction(hash))){
                    delete tx;
                    return true;
                }
            } else if(item.IsBlock()){
                Block* blk;
                if((blk = BlockChain::GetBlockData(hash))){
                    delete blk;
                    return true;
                }

                if((blk = BlockPool::GetBlock(hash))){
                    delete blk;
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
            pthread_mutex_lock(&rmutex_);
            while(!IsResolved(item)) pthread_cond_wait(&rcond_, &rmutex_);
            pthread_mutex_unlock(&rmutex_);
        }

        void WaitForItems(std::vector<InventoryItem>& items){
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

        static void OnMessageSent(uv_write_t* req, int status){
            if(status != 0) LOG(ERROR) << "failed to send message: " << uv_strerror(status);
            if(req) free(req);
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

        void Send(const Message& msg){
            pthread_mutex_trylock(&smutex_);
            LOG(INFO) << "sending " << msg;
            uint32_t rtype = static_cast<uint32_t>(msg.GetType());
            uint64_t rsize = msg.GetSize();
            uint64_t total_size = Message::kHeaderSize + rsize;

            uint8_t bytes[total_size];
            memcpy(&bytes[Message::kTypeOffset], &rtype, Message::kTypeLength);
            memcpy(&bytes[Message::kSizeOffset], &rsize, Message::kSizeLength);
            msg.Encode(&bytes[Message::kDataOffset], rsize);

            uv_buf_t buff = uv_buf_init((char*)bytes, total_size);
            uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
            req->data = this;
            uv_write(req, GetHandle(), &buff, 1, &OnMessageSent);
            pthread_mutex_unlock(&smutex_);
        }
    };

    class PeerSession : public Session{
    private:
        pthread_t thread_;
        uv_connect_t conn_;
        uv_tcp_t socket_;
        std::string node_id_;
        NodeAddress node_addr_;

        static void* PeerSessionThread(void* data);
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnMessageSent(uv_write_t *req, int status);

#define DECLARE_HANDLER(Name) \
    static void Handle##Name##Message(uv_work_t* handle);
        FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLER);
        static void AfterHandleMessage(uv_work_t* handle, int status);

        static void HandleGetDataTask(uv_work_t* handle);
        static void AfterHandleGetDataTask(uv_work_t* handle, int status);
    protected:
        uv_stream_t* GetStream() const{
            return conn_.handle;
        }
    public:
        PeerSession(const NodeAddress& addr):
                thread_(),
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

        void Send(const Message& msg);

        bool Connect(){
            return pthread_create(&thread_, NULL, &PeerSessionThread, (void*)this) == 0;
        }
    };

    //TODO: ClientSession
    class NodeClient : public Session{
    private:
        uv_loop_t* loop_;
        uv_signal_t sigterm_;
        uv_signal_t sigint_;
        uv_tcp_t stream_;
        uv_connect_t connection_;
        uv_pipe_t stdin_;
        uv_pipe_t stdout_;

        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnCommandReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnMessageSent(uv_write_t *req, int status);
        static void OnSignal(uv_signal_t* handle, int signum);

#define DECLARE_HANDLER_FUNCTION(Name) \
    static void Handle##Name##Message(uv_work_t* handle);
        FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLER_FUNCTION);
        static void AfterHandleMessage(uv_work_t* handle, int status);

        inline uv_stream_t*
        GetStream(){
            return (uv_stream_t*)&stream_;
        }

        inline uv_stream_t*
        GetStdinPipe(){
            return (uv_stream_t*)&stdin_;
        }
    public:
        NodeClient():
                loop_(uv_loop_new()),
                sigterm_(),
                sigint_(),
                stream_(),
                stdin_(),
                stdout_(){
            stream_.data = this;
            connection_.data = this;
        }
        ~NodeClient(){}

        void Connect(const NodeAddress& addr);
    };
}

#endif //TOKEN_SESSION_H