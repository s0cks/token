#ifndef TOKEN_PEER_SESSION_H
#define TOKEN_PEER_SESSION_H

#include <utility>

#include "network/rpc_message.h"
#include "network/rpc_session.h"
#include "inventory.h"

namespace token{
  class PeerSession;
  typedef std::shared_ptr<PeerSession> PeerSessionPtr;

  class Peer{
   public:
    static inline int64_t
    GetSize(){
      return UUID::GetSize()
           + NodeAddress::kSize;
    }

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

  class PeerMessageHandler : public rpc::MessageHandler{
    friend class PeerSession;
   private:
    explicit PeerMessageHandler(PeerSession* session):
      rpc::MessageHandler((rpc::Session*)session){}
   public:
    ~PeerMessageHandler() override = default;

  #define DECLARE_HANDLE(Name) \
    void On##Name##Message(const rpc::Name##MessagePtr& msg) override;

    FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLE)
  #undef DECLARE_HANDLE
  };

  //TODO: refactor
  class PeerSession : public rpc::Session{
    friend class PeerSessionThread;
    friend class PeerSessionManager;
   private:
    BlockChainPtr chain_;

    Peer info_;
    BlockHeader head_;
    // Disconnect
    uv_async_t disconnect_;
    // Needed for Paxos
    uv_async_t discovery_;
    uv_async_t prepare_;
    uv_async_t promise_;
    uv_async_t commit_;
    uv_async_t accepted_;
    uv_async_t rejected_;

    // Messages
    PeerMessageHandler handler_;

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
    static void OnDiscovery(uv_async_t* handle);
    // Disconnect
    static void OnWalk(uv_handle_t* handle, void* arg);
    static void OnClose(uv_handle_t* handle);

    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
      buf->base = (char*)malloc(suggested_size);
      buf->len = suggested_size;
    }

   protected:
    PeerMessageHandler& GetMessageHandler() override{
      return handler_;
    }

    BlockChainPtr GetChain() const{
      return chain_;
    }

    bool ItemExists(const InventoryItem& item) const{
      return true;//TODO: refactor
    }

    void OnMessageRead(const BufferPtr& buff){
      rpc::MessagePtr msg = rpc::Message::From(buff);
      switch(msg->type()) {
#define DEFINE_HANDLE(Name) \
        case Type::k##Name##Message: \
          return GetMessageHandler().On##Name##Message(std::static_pointer_cast<rpc::Name##Message>(msg));

        FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLE)
#undef DEFINE_HANDLE
        default: return;
      }
    }
   public:
    explicit PeerSession(const NodeAddress& address, BlockChainPtr chain):
      rpc::Session(uv_loop_new()),
      chain_(std::move(chain)),
      info_(UUID(), address),
      head_(),
      disconnect_(),
      discovery_(),
      prepare_(),
      promise_(),
      commit_(),
      accepted_(),
      rejected_(),
      handler_(this){
      // core
      disconnect_.data = this;
      CHECK_UVRESULT(uv_async_init(loop_, &disconnect_, &OnDisconnect), LOG(ERROR), "cannot initialize the OnDisconnect callback");
      // paxos
      discovery_.data = this;
      CHECK_UVRESULT(uv_async_init(loop_, &discovery_, &OnDiscovery), LOG(ERROR), "cannot initialize the OnDiscovery callback");
      prepare_.data = this;
      CHECK_UVRESULT(uv_async_init(loop_, &prepare_, &OnPrepare), LOG(ERROR), "cannot initialize the OnPrepare callback");
      promise_.data = this;
      CHECK_UVRESULT(uv_async_init(loop_, &promise_, &OnPromise), LOG(ERROR), "cannot initialize the OnPromise callback");
      commit_.data = this;
      CHECK_UVRESULT(uv_async_init(loop_, &commit_, &OnCommit), LOG(ERROR), "cannot initialize the OnCommit callback");
      accepted_.data = this;
      CHECK_UVRESULT(uv_async_init(loop_, &accepted_, &OnAccepted), LOG(ERROR), "cannot initialize the OnAccepted callback");
      rejected_.data = this;
      CHECK_UVRESULT(uv_async_init(loop_, &rejected_, &OnRejected), LOG(ERROR), "cannot initialize the OnRejected callback");
    }
    ~PeerSession() override = default;

    Peer GetInfo() const{
      return info_;
    }

    //TODO: remove
    UUID GetID() const{
      return info_.GetID();
    }

    NodeAddress GetAddress() const{
      return info_.GetAddress();
    }

    void SendDiscoveredBlock(){
      uv_async_send(&discovery_);
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "PeerSession(" << GetID() << ")";
      return ss.str();
    }
  };
}

#endif //TOKEN_PEER_SESSION_H