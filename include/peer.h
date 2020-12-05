#ifndef TOKEN_PEER_H
#define TOKEN_PEER_H

#include <set>
#include "uuid.h"
#include "block.h"
#include "session.h"
#include "address.h"
#include "message.h"

namespace Token{
    class Peer{
    public:
        static const intptr_t kSize = UUID::kSize + NodeAddress::kSize;

        struct IDComparator{
            bool operator()(const Peer& a, const Peer& b){
                return a.uuid_ < b.uuid_;
            }
        };

        struct AddressComparator{
            bool operator()(const Peer& a, const Peer& b){
                return a.address_ < b.address_;
            }
        };
    private:
        UUID uuid_;
        NodeAddress address_;
    public:
        Peer(const UUID& uuid, const NodeAddress& address):
            uuid_(uuid),
            address_(address){}
        Peer(Buffer* buff):
            uuid_(buff),
            address_(buff){}
        Peer(const Peer& other):
            uuid_(other.uuid_),
            address_(other.address_){}
        ~Peer() = default;

        UUID GetID() const{
            return uuid_;
        }

        NodeAddress GetAddress() const{
            return address_;
        }

        bool Write(Buffer* buff) const{
            uuid_.Write(buff);
            address_.Write(buff);
            return true;
        }

        void operator=(const Peer& other){
            uuid_ = other.uuid_;
            address_ = other.address_;
        }

        friend bool operator==(const Peer& a, const Peer& b){
            return a.uuid_ == b.uuid_
                   && a.address_ == b.address_;
        }

        friend bool operator!=(const Peer& a, const Peer& b){
            return !operator==(a, b);
        }

        friend bool operator<(const Peer& a, const Peer& b){
            return a.address_ < b.address_;
        }

        friend std::ostream& operator<<(std::ostream& stream, const Peer& peer){
            stream << peer.GetID() << "(" << peer.GetAddress() << ")";
            return stream;
        }
    };

    class HandleMessageTask;
    class PeerSession : public Session{
        friend class Server;
        friend class PeerSessionThread;
        friend class PeerSessionManager;
    private:
        pthread_t thread_;
        Peer info_;
        BlockHeader head_;
        uv_async_t disconnect_;
        // Needed for Paxos
        uv_async_t prepare_;
        uv_async_t promise_;
        uv_async_t commit_;
        uv_async_t accepted_;
        uv_async_t rejected_;

        void SetInfo(const Peer& info){
            info_ = info;
        }

        void StartHeartbeatTimer(){
            //uv_timer_start(&session->hb_timer_, &OnHeartbeatTick, 90 * 1000, Session::kHeartbeatIntervalMilliseconds);
        }

        bool Connect();
        bool Disconnect();

        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void OnDisconnect(uv_async_t* handle);
        // Paxos
        static void OnPrepare(uv_async_t* handle);
        static void OnPromise(uv_async_t* handle);
        static void OnCommit(uv_async_t* handle);
        static void OnAccepted(uv_async_t* handle);
        static void OnRejected(uv_async_t* handle);

#define DECLARE_MESSAGE_HANDLER(Name) \
        static void Handle##Name##Message(HandleMessageTask* task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
    public:
        PeerSession(uv_loop_t* loop, const NodeAddress& address):
            Session(loop),
            thread_(pthread_self()),
            info_(UUID(), address),
            head_(),
            disconnect_(),
            prepare_(),
            promise_(),
            commit_(),
            accepted_(),
            rejected_(){
            disconnect_.data = this;
            prepare_.data = this;
            promise_.data = this;
            commit_.data = this;
            accepted_.data = this;
            rejected_.data = this;
            uv_async_init(loop, &disconnect_, &OnDisconnect);
            uv_async_init(loop, &prepare_, &OnPrepare);
            uv_async_init(loop, &promise_, &OnPromise);
            uv_async_init(loop, &commit_, &OnCommit);
            uv_async_init(loop, &accepted_, &OnAccepted);
            uv_async_init(loop, &rejected_, &OnRejected);
        }
        ~PeerSession() = default;

        Peer GetInfo() const{
            return info_;
        }

        UUID GetID() const{
            return info_.GetID();
        }

        NodeAddress GetAddress() const{
            return info_.GetAddress();
        }

        void SendPrepare(){
            uv_async_send(&prepare_);
        }

        void SendPromise(){
            uv_async_send(&promise_);
        }

        void SendAccepted(){
            uv_async_send(&accepted_);
        }

        void SendRejected(){
            uv_async_send(&rejected_);
        }

        void SendCommit(){
            uv_async_send(&commit_);
        }
    };

    class PeerSessionManager{
    private:
        PeerSessionManager() = delete;
    public:
        ~PeerSessionManager() = delete;

        static bool Initialize();
        static bool Shutdown();
        static bool IsConnectedTo(const UUID& uuid);
        static bool IsConnectedTo(const NodeAddress& address);
        static bool ConnectTo(const NodeAddress& address);
        static int32_t GetNumberOfConnectedPeers();
        static std::shared_ptr<PeerSession> GetSession(const UUID& uuid);
        static std::shared_ptr<PeerSession> GetSession(const NodeAddress& address);
        static void BroadcastPrepare();
        static void BroadcastCommit();
    };
}

#endif //TOKEN_PEER_H
