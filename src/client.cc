#include "client.h"
#include "uuid.h"
#include "sys/socket.h"
#include "unistd.h"
#include "arpa/inet.h"

namespace Token{
    void* Client::ClientThread(void* args){
        Client* client = (Client*)args;

        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        inet_pton(AF_INET, client->address_.c_str(), &address.sin_addr);
        address.sin_port = htons(client->port_);
        int sock;
        if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
            std::cerr << "Couldn't create the socket" << std::endl;
            return nullptr;
        }
        if(connect(sock, (struct sockaddr*)&address, sizeof(address)) < 0){
            std::cerr << "Couldn't connect to server" << std::endl;
            return nullptr;
        }



        close(sock);
        return nullptr;
    }

    void Client::Connect(const std::string& addr, int port){
        address_ = addr;
        port_ = port;
        pthread_create(&thread_id_, NULL, ClientThread, (void*)this);
    }
}