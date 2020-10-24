#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include "session.h"
#include "command.h"

namespace Token{
#define FOR_EACH_CLIENT_COMMAND(V) \
    V(Status, ".status", 0) \
    V(Disconnect, ".disconnect", 0) \
    V(Transaction, "tx", 3)

    class ClientSession : public Session{
        //TODO: add client command handler
    private:
        pthread_t thread_;
        uv_signal_t sigterm_;
        uv_signal_t sigint_;
        uv_tcp_t stream_;
        uv_timer_t hb_timer_;
        uv_timer_t hb_timeout_;
        uv_async_t shutdown_;
        NodeAddress paddress_;

        void SetPeerAddress(const NodeAddress& address){
            paddress_ = address;
        }

        static void* ClientSessionThread(void* data);
        static void OnShutdown(uv_async_t* handle);
        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnSignal(uv_signal_t* handle, int signum);
        static void OnHeartbeatTick(uv_timer_t* handle);
        static void OnHeartbeatTimeout(uv_timer_t* handle);

        virtual uv_loop_t* GetLoop(){
            return stream_.loop;
        }

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&stream_;
        }
    public:
        ClientSession(const NodeAddress& address);
        ~ClientSession(){}

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(const Handle<HandleMessageTask>& task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

        NodeAddress GetPeerAddress() const {
            return paddress_;
        }

        bool Connect();
        void Disconnect();
    };

    class BlockChainClient{
    private:
        ClientSession* session_;

        ClientSession* GetSession() const{
            return session_;
        }
    public:
        BlockChainClient(const NodeAddress& address):
            session_(new ClientSession(address)){}
        ~BlockChainClient() = default;

        bool Connect();
        bool Disconnect();
        bool GetPeers(PeerList& peers);
        bool GetUnclaimedTransactions(const User& user, std::vector<Hash>& utxos);
        bool Send(const Handle<Transaction>& tx);
        Handle<UnclaimedTransaction> GetUnclaimedTransaction(const Hash& hash);
    };
}

#endif //TOKEN_CLIENT_H
