#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include <vector>
#include <string>
#include <cstdint>
#include <pthread.h>
#include <sys/socket.h>
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

    class BlockChainNode{
    private:
        pthread_t thread_;
        std::string address_;
        uint16_t port_;
        uint32_t sock_;
        std::vector<NodeServerSession*> sessions_;

        void SetAddress(const std::string& addr){
            address_ = addr;
        }

        void SetPort(uint16_t port){
            port_ = port;
        }

        uint32_t GetSocket(){
            return sock_;
        }

        void CloseSessions();

        static void* ServerThread(void* data);

        BlockChainNode();
    public:
        ~BlockChainNode();

        const char* GetAddress(){
            return address_.c_str();
        }

        uint16_t GetPort(){
            return port_;
        }

        static BlockChainNode* GetInstance();
        static void Initialize(std::string addr, uint16_t port);
    };
}

#endif //TOKEN_NODE_H
