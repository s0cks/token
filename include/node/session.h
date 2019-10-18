#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <pthread.h>
#include <string>
#include <cstdint>

#include "message.h"
#include "blockchain.h"

namespace Token{
    class BlockChainNode;

    enum class SessionState{
        kConnecting,
        kConnected,
        kDisconnected
    };

    class Session{
    private:
        SessionState state_;
    protected:
        void SetState(SessionState state){
            state_ = state;
        }

        Session():
            state_(SessionState::kDisconnected){

        }
    public:
        virtual ~Session(){}
        virtual void Send(Message* msg) = 0;

        void SendBlock(Block* block);

        SessionState GetState(){
            return state_;
        }
    };

    class NodeServerSession : public Session{
    private:
        BlockChainNode* parent_;
        pthread_t thread_;
        uint32_t sock_;

        uint32_t GetSocket(){
            return sock_;
        }

        bool Handle(Message* msg);
        void StartThread();
        static void* SessionThread(void* data);

        friend class BlockChainNode;

        NodeServerSession(BlockChainNode* server, uint32_t sock);
    public:
        ~NodeServerSession();

        void Send(Message* msg);
    };

    class NodeClientSession : public Session{
    private:
        pthread_t thread_;
        std::string address_;
        uint32_t sock_;
        uint16_t port_;

        static void* ClientThread(void* data);
    public:
        NodeClientSession(const std::string& address, uint16_t port);
        ~NodeClientSession();

        const char* GetAddress(){
            return address_.c_str();
        }

        uint16_t GetPort(){
            return port_;
        }

        void Send(Message* msg);
        void Connect();
    };
}

#endif //TOKEN_SESSION_H