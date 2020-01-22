#include <glog/logging.h>
#include <string.h>
#include <netinet/in.h>

#include "node/node.h"

namespace Token{
    BlockChainNode::BlockChainNode():
            sessions_(),
            address_(),
            port_(0),
            thread_(){
    }

    BlockChainNode::~BlockChainNode(){

    }

    BlockChainNode* BlockChainNode::GetInstance(){
        static BlockChainNode instance;
        return &instance;
    }

    void* BlockChainNode::ServerThread(void* data){
        BlockChainNode* server = (BlockChainNode*)data;
        std::string err_msg = "";

        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(server->GetPort());
        address.sin_addr.s_addr = htonl(INADDR_ANY);

        if((server->sock_ = socket(PF_INET, SOCK_STREAM, 0)) < 0){
            err_msg = "cannot create server";
            goto exit; //TODO: Fixme
        }

        if(bind(server->GetSocket(), (struct sockaddr*)&address, sizeof(address)) < 0){
            err_msg = "cannot bind server socket";
            goto exit; //TODO: Fixme
        }

        if(listen(server->GetSocket(), 16) < 0){
            err_msg = "cannot listen on socket";
            goto exit; //TODO: fixmeq
        }

        while(true){
            uint32_t client;
            struct sockaddr_in client_address;
            int client_address_len;

            if((client = accept(server->GetSocket(), (struct sockaddr*)&client_address, (socklen_t*)&client_address_len)) < 0){
                LOG(WARNING) << "couldn't accept client";
                goto exit; //TODO: Fixme
            }

            NodeServerSession* session = new NodeServerSession(server, client);
            session->StartThread();
        }

        exit:
        server->CloseSessions();
        char* result = strdup(err_msg.c_str());
        pthread_exit(result);
    }

    void BlockChainNode::CloseSessions(){
        for(auto& it : sessions_){
            delete it;
        }
    }

    void BlockChainNode::Initialize(std::string addr, uint16_t port){
        GetInstance()->SetAddress(addr);
        GetInstance()->SetPort(port);
        pthread_create(&GetInstance()->thread_, NULL, &ServerThread, GetInstance());
    }

    void BlockChainNode::WaitForShutdown(){
        char message[4096];
        pthread_join(GetInstance()->thread_, (void**)&message);
        LOG(INFO) << "server exited with: " << message;
    }
}