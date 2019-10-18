#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <pthread.h>
#include <string>
#include <cstdint>

#include "message.h"

namespace Token{
    class BlockChainNode;

    class NodeServerSession{
    private:
        BlockChainNode* parent_;
        pthread_t thread_;
        uint32_t sock_;

        uint32_t GetSocket(){
            return sock_;
        }

        void StartThread();

        static void* SessionThread(void* data);

        friend class BlockChainNode;

        NodeServerSession(BlockChainNode* server, uint32_t sock):
                thread_(),
                parent_(server),
                sock_(sock){}
    public:
        ~NodeServerSession();

        void Send(Message* msg);
    };

    class NodeClientSession{
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