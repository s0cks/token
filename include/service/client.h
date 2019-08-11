
#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include <grpc++/grpc++.h>
#include "service.h"

namespace Token{
    class TokenServiceClient{
    private:
        std::shared_ptr<grpc::Channel> channel_;
        std::unique_ptr<Token::Service::BlockChainService::Stub> stub_;

        inline Token::Service::BlockChainService::Stub*
        GetStub(){
            return stub_.get();
        }
    public:
        TokenServiceClient(const std::string& address, int port);
        ~TokenServiceClient(){}

        bool GetHead(Messages::BlockHeader* response);
        bool GetBlock(const std::string& hash, Messages::BlockHeader* response);
        bool GetBlockAt(int index, Messages::BlockHeader* response);
        bool Append(Block* block, Messages::BlockHeader* response);
    };
}

#endif //TOKEN_CLIENT_H