#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include <pthread.h>
#include <string>
#include <vector>
#include <cstdint>

#include "session.h"

namespace Token{
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
