#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glog/logging.h>
#include <cstdio>

#include "node/session.h"
#include "blockchain.h"

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

        ByteArray bytes(4096);
        int len;
        while((len = recv(session->GetSocket(), (char*)bytes.Data(), bytes.Capacity(), 0)) >= 0){
            if(len == 0) continue;
            Message msg(&bytes);
            LOG(INFO) << "decoded msg: " << msg.ToString();
            if(!session->Handle(&msg)){
                LOG(ERROR) << "couldn't handle message: " << msg.ToString();
                goto exit;
            }
            bytes.Clear();
        }

        exit:
            char* result = strdup(err_msg.c_str());
            pthread_exit(result);
    }

    bool NodeServerSession::Handle(Token::Message* msg){
        if(msg->IsBlockMessage()){
            //TODO: Block* blk = Block::Decode(msg->GetAsBlock());
            // LOG(INFO) << "received block: " << blk->GetHash();
            return true;
        }
        return false;
    }

    void NodeServerSession::Send(Token::Message* msg){
        ByteArray bytes(msg->GetMessageSize());
        if(!msg->Encode(&bytes)){
            LOG(ERROR) << "couldn't encode message: " << msg->ToString();
            return;
        }
        send(sock_, bytes.Data(), bytes.Length(), 0);
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
        Message msg(block);
        Send(&msg);
    }

    bool NodeClientSession::Handle(Token::Message* msg){
        LOG(INFO) << "received: " << msg->ToString();
        return true;
    }

    void NodeClientSession::Send(Token::Message* msg){
        LOG(INFO) << "sending: " << msg->ToString();
        ByteArray bytes(msg->GetMessageSize() + 1);
        if(!msg->Encode(&bytes)){
            LOG(ERROR) << "couldn't encode message: " << msg->ToString();
            return;
        }
        bytes[msg->GetMessageSize()] = '\0';
        send(sock_, bytes.Data(), bytes.Length(), 0);
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

        ByteArray bytes(4096);
        int len;
        while((len = recv(client->sock_, (char*)bytes.Data(), bytes.Capacity(), 0)) > 0) {
            if(len == 0) continue;
            Message msg(&bytes);
            if(!client->Handle(&msg)){
                LOG(ERROR) << "couldn't handle: " << msg.ToString();
                break;
            }
            bytes.Clear();
        }
    }

    void NodeClientSession::Connect(){
        pthread_create(&thread_, NULL, &ClientThread, this);
    }
}