
#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include <grpc++/grpc++.h>
#include "service.h"

namespace Token{
    class TokenServiceClient{
    private:
        std::shared_ptr<grpc::Channel> channel_;
        std::unique_ptr<Token::Messages::Service::BlockChainService::Stub> stub_;

        inline Token::Messages::Service::BlockChainService::Stub*
        GetStub(){
            return stub_.get();
        }
    public:
        TokenServiceClient(const std::string& address, int port);
        TokenServiceClient(const std::string& address);
        ~TokenServiceClient(){}

        bool GetHead(Messages::BlockHeader* response);
        bool GetBlock(const std::string& hash, Messages::BlockHeader* response);
        bool GetUnclaimedTransactions(Messages::UnclaimedTransactionList* response);
        bool GetUnclaimedTransactions(const std::string& hash, Messages::UnclaimedTransactionList* response);
        bool SpendToken(const std::string& token, const std::string& from_user, const std::string& to_user, Transaction** response);
    };
}

#endif //TOKEN_CLIENT_H