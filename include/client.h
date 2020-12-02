#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include "session.h"
#include "command.h"

namespace Token{
    class ClientSession : public ThreadedSession{
    private:
        NodeAddress address_;
        UUID id_;
        Message* next_;

        ClientSession(uv_loop_t* loop, const NodeAddress& address):
            ThreadedSession(loop),
            address_(address),
            id_(),
            next_(nullptr){}

        void SetNextMessage(Message* msg);
        void WaitForNextMessage();
        Message* GetNextMessage();
        static void OnConnect(uv_connect_t* conn, int status);
        static void OnShutdown(uv_async_t* handle);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void* SessionThread(void* data);
    public:
        bool Connect(){
            int err;
            if((err = pthread_create(&thread_, NULL, &SessionThread, this)) != 0){
                LOG(WARNING) << "couldn't start session thread: " << strerror(err);
                return false;
            }
            return true;
        }

        NodeAddress GetAddress() const{
            return address_;
        }

        UUID GetID() const{
            return id_;
        }

        Block* GetBlock(const Hash& hash);
        bool GetBlockChain(std::set<Hash>& blocks);
        bool GetPeers(PeerList& peers);
        bool GetUnclaimedTransactions(const User& user, std::vector<Hash>& utxos);
        bool SendTransaction(Transaction* tx);
        UnclaimedTransaction* GetUnclaimedTransaction(const Hash& hash);

        static ClientSession* NewInstance(uv_loop_t* loop, const NodeAddress& address){
            return new ClientSession(loop, address);
        }

        static ClientSession* NewInstance(const NodeAddress& address){
            return new ClientSession(uv_loop_new(), address);
        }
    };
}

#endif //TOKEN_CLIENT_H
