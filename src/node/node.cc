#include <string.h>
#include <glog/logging.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <node/message.h>
#include <block.h>
#include <blockchain.h>
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



    NodeServerSession::~NodeServerSession(){
        close(GetSocket());
    }

    void* NodeServerSession::SessionThread(void* data){
        NodeServerSession* session = (NodeServerSession*)data;
        std::string err_msg = "";

        uint32_t buffer_len = 4096;
        uint8_t* buffer = (uint8_t*)malloc(sizeof(uint8_t) * buffer_len);
        int len;
        while((len = recv(session->GetSocket(), (char*)buffer, buffer_len, 0)) >= 0){
            if(len == 0) continue;
            Message msg;
            if(!msg.Decode(buffer, 0)){
                LOG(ERROR) << "couldn't decode message";
                break;
            }

            if(msg.IsBlockMessage()){
                Block* blk = Block::Decode(msg.GetAsBlock());
                LOG(INFO) << "received block: " << blk->GetHash();

                Message resp(Message::Type::kBlockMessage, blk->GetAsMessage()); // Echo
                session->Send(&resp);
            } else{
                LOG(WARNING) << "unknown message type: " << msg.ToString();
                goto exit;
            }

            memset(buffer, 0, sizeof(char) * buffer_len);
        }

        exit:
            char* result = strdup(err_msg.c_str());
            pthread_exit(result);
    }

    void NodeServerSession::Send(Token::Message* msg){
        LOG(WARNING) << "sending: " << msg->ToString();
        size_t size = msg->GetMessageSize();
        uint8_t bytes[size + 1];
        bytes[size] = '\0';
        send(sock_, bytes, size, 0);
    }

    void NodeServerSession::StartThread(){
        pthread_create(&thread_, NULL, &SessionThread, this);
    }



    NodeClientSession::NodeClientSession(const std::string &address, uint16_t port):
            address_(address),
            thread_(),
            sock_(),
            port_(port){

    }

    NodeClientSession::~NodeClientSession(){

    }

    void NodeClientSession::Send(Token::Message* msg){
        size_t size = msg->GetMessageSize();
        uint8_t bytes[size + 1];
        if(!msg->Encode(bytes, size)){
            LOG(WARNING) << "couldn't encode message: " << msg->ToString();
            return;
        }
        bytes[size] = '\0';
        send(sock_, bytes, size, 0);
    }

    void* NodeClientSession::ClientThread(void* data){
        NodeClientSession* client = (NodeClientSession*)data;

        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        inet_pton(AF_INET, client->GetAddress(), &address.sin_addr);
        address.sin_port = htons(client->GetPort());

        int rc;
        if((client->sock_ = socket(PF_INET, SOCK_STREAM, 0)) < 0){
            LOG(WARNING) << "couldn't open client socket";
            return nullptr; //TODO: Fixme
        }

        if((rc = connect(client->sock_, (struct sockaddr*)&address, sizeof(address))) < 0){
            LOG(WARNING) << "couldn't connect to server: " << strerror(rc);
            return nullptr; //TODO: Fixme
        }

        Message head(Message::Type::kBlockMessage, BlockChain::GetHead()->GetAsMessage());
        client->Send(&head);

        uint32_t buffer_len = 4096;
        uint8_t* buffer = (uint8_t*)malloc(sizeof(uint8_t) * buffer_len);
        int len;
        while((len = recv(client->sock_, (char*)buffer, buffer_len, 0)) > 0) {
            if(len == 0) continue;
            Message msg;
            if(!msg.Decode(buffer, 0)){
                LOG(WARNING) << "couldn't decode message";
                return nullptr; //TODO: Fixme
            }

            if(msg.IsBlockMessage()){
                Block* blk = Block::Decode(msg.GetAsBlock());
                LOG(INFO) << "received block: " << blk->GetHash();
            }
            memset(buffer, 0, sizeof(uint8_t) * buffer_len);
        }
    }

    void NodeClientSession::Connect(){
        pthread_create(&thread_, NULL, &ClientThread, this);
    }
}