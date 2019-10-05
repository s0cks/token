
#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include <grpc++/grpc++.h>
#include "service.h"

namespace Token{
    class TokenServiceClient{
    private:
        std::shared_ptr<grpc::Channel> channel_;
        std::unique_ptr<Token::Service::Messages::BlockChainService::Stub> stub_;

        inline Token::Service::Messages::BlockChainService::Stub*
        GetStub(){
            return stub_.get();
        }
    public:
        TokenServiceClient(const std::string& address, int port);
        ~TokenServiceClient(){}

        bool GetHead(Messages::BlockHeader* response);
        bool GetBlock(const std::string& hash, Messages::BlockHeader* response);
        bool GetBlockAt(int index, Messages::BlockHeader* response);
        bool GetBlockData(const std::string& hash, Messages::Block* response);
        bool GetBlockDataAt(int index, Messages::Block* response);
        bool GetUnclaimedTransactions(Messages::UnclaimedTransactionList* response);
        bool GetUnclaimedTransactions(const std::string& hash, Messages::UnclaimedTransactionList* response);
        bool GetPeers(std::vector<std::string>& peers);
        bool Spend(const std::string& token, const std::string& from_user, const std::string& to_user);
    };
}

#endif //TOKEN_CLIENT_H