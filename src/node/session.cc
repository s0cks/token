#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glog/logging.h>
#include <cstdlib>
#include <cstdio>

#include "node/session.h"
#include "blockchain.h"


/*
 * #include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
 *
 */

namespace Token{
    NodeServerSession::NodeServerSession(BlockChainNode* server, uint32_t sock):
        thread_(),
        parent_(server),
        sock_(sock),
        Session(){

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

            if(!session->Handle(&msg)){
                LOG(ERROR) << "couldn't handle message: " << msg.ToString();
                goto exit;
            }
            memset(buffer, 0, sizeof(char) * buffer_len);
        }

        exit:
            char* result = strdup(err_msg.c_str());
            pthread_exit(result);
    }

    bool NodeServerSession::Handle(Token::Message* msg){
        if(msg->IsBlockMessage()){
            Block* blk = Block::Decode(msg->GetAsBlock());
            LOG(INFO) << "received block: " << blk->GetHash();
            return true;
        }

        return false;
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
            port_(port),
            Session(){

    }

    NodeClientSession::~NodeClientSession(){

    }

    void Session::SendBlock(Token::Block* block){
        Message msg(Message::Type::kBlockMessage, block->GetAsMessage());
        Send(&msg);
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
        LOG(INFO) << "connecting to peer: " << client->GetAddress() << ":" << client->GetPort() << "....";
        client->SetState(SessionState::kConnecting);

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

        LOG(INFO) << "connected!";
        client->SetState(SessionState::kConnected);
        client->SendBlock(BlockChain::GetHead());

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