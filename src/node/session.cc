#include <glog/logging.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include "node/session.h"
#include "node/message.h"

namespace Token{
    enum{
        kTypeOffset = 0,
        kTypeLength = 1,
        kSizeOffset = kTypeLength,
        kSizeLength = 4,
        kDataOffset = (kTypeLength + kSizeLength),
        kHeaderSize = kDataOffset,
        kByteBufferSize = 4096
    };

    static inline void EncodeSize(uint32_t size, uint8_t* bytes){
        bytes[kSizeOffset] = size & 0xFF;
        bytes[kSizeOffset + 1] = (size >> 8) & 0xFF;
        bytes[kSizeOffset + 2] = (size >> 16) & 0xFF;
        bytes[kSizeOffset + 3] = (size >> 24) & 0xFF;
    }

    static inline uint32_t DecodeSize(uint8_t* bytes){
        return (bytes[kSizeOffset]) |
                (bytes[kSizeOffset + 1] << 8) |
                (bytes[kSizeOffset + 2] << 16) |
                (bytes[kSizeOffset + 3] << 24);
    }

    NodeServerSession::NodeServerSession(BlockChainNode* server, uint32_t sock):
            thread_(),
            parent_(server),
            sock_(sock),
            Session(){}

    NodeServerSession::~NodeServerSession(){
        close(sock_);
    }

    void* NodeServerSession::SessionThread(void* data){
        std::stringstream err_msg;
        NodeServerSession* session = (NodeServerSession*)data;
        uint8_t bytes[kByteBufferSize];
        int len;
        while((len = recv(session->GetSocket(), (char*)bytes, kByteBufferSize, 0)) >= 0) {
            if (len == 0) continue;
            Message::Type type = static_cast<Message::Type>(bytes[kTypeOffset]);
            uint32_t size = DecodeSize(bytes);
            Message* message = Message::Decode(type, &bytes[kDataOffset], size);
            if(!message){
                err_msg << "session couldn't decode the message";
                goto exit;
            }
            if(!session->Handle(message)){
                err_msg << "session couldn't handle message: " << message->GetName();
                goto exit;
            }
            memset(bytes, 0, kByteBufferSize);
        }

        exit:
            std::string error_message = err_msg.str();
            pthread_exit(strdup(error_message.data()));
    }

    bool NodeServerSession::Handle(Token::Message* msg){
        if(msg->IsPingMessage()){
            std::string nonce = msg->AsPingMessage()->GetNonce();
            LOG(INFO) << "received ping " << nonce << ", sending pong";
            SendPong(nonce);
            return true;
        }
        return false;
    }

    void Session::Send(Token::Message* msg){
        LOG(INFO) << "sending " << msg->GetName();
        size_t size = msg->GetSize();
        uint8_t bytes[kHeaderSize + size];
        bytes[kTypeOffset] = static_cast<uint8_t>(msg->GetType());
        EncodeSize(size, bytes);
        msg->Encode(&bytes[kDataOffset], size);
        send(GetSocket(), bytes, kHeaderSize + size, 0);
    }

    void Session::SendPing(const std::string& nonce){
        PingMessage msg(nonce);
        Send(&msg);
    }

    void Session::SendPong(const std::string& nonce){
        PongMessage msg(nonce);
        Send(&msg);
    }

    void NodeServerSession::StartThread(){
        pthread_create(&thread_, NULL, &SessionThread, this);
    }

    NodeClientSession::NodeClientSession(const std::string &address, uint16_t port):
            address_(address),
            thread_(),
            sock_(),
            port_(port),
            Session(){}

    NodeClientSession::~NodeClientSession(){
        close(sock_);
    }

    void Session::SendTransaction(Token::Transaction* tx){
        TransactionMessage msg(tx);
        Send(&msg);
    }

    void Session::SendBlock(Token::Block* block){
        BlockMessage msg(block);
        Send(&msg);
    }

    bool NodeClientSession::Handle(Token::Message* msg){
        return true;
    }

    void* NodeClientSession::ClientThread(void* data){
        std::stringstream err_msg;
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
            err_msg << "couldn't open client socket";
            goto exit;
        }

        if((rc = connect(client->sock_, (struct sockaddr*)&address, sizeof(address))) < 0){
            err_msg << "couldn't connect to server: " << strerror(rc);
            goto exit;
        }

        LOG(INFO) << "connected!";
        client->SetState(SessionState::kConnected);
        client->SendPing();

        uint8_t bytes[kByteBufferSize];
        int len;
        while((len = recv(client->GetSocket(), (char*)bytes, kByteBufferSize, 0)) >= 0) {
            if (len == 0) continue;
            Message::Type type = static_cast<Message::Type>(bytes[kTypeOffset]);
            uint32_t size = DecodeSize(bytes);
            Message* message = Message::Decode(type, &bytes[kDataOffset], size);
            if(!message){
                err_msg << "session couldn't decode message";
                goto exit;
            }
            if(!client->Handle(message)){
                err_msg << "session couldn't handle message: " << message->GetName();
                goto exit;
            }
            memset(bytes, 0, kByteBufferSize);
        }

        exit:
            std::string error_message = err_msg.str();
            pthread_exit(strdup(error_message.data()));
    }

    void NodeClientSession::Connect(){
        pthread_create(&thread_, NULL, &ClientThread, this);
    }
}