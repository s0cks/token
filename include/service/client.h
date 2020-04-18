
#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include <grpc++/grpc++.h>
#include "service.h"
#include "block_chain.h"

using namespace Token::Proto::BlockChainService;
using namespace Token::Proto::BlockChain;

namespace Token{
    class TokenServiceClient{
    private:
        std::shared_ptr<grpc::Channel> channel_;
        std::unique_ptr<::BlockChainService::Stub> stub_;

        inline ::BlockChainService::Stub*
        GetStub(){
            return stub_.get();
        }
    public:
        TokenServiceClient(const std::string& address, int port);
        TokenServiceClient(const std::string& address);
        ~TokenServiceClient(){}

        bool GetHead(::BlockHeader* response);
        bool GetBlock(const std::string& hash, ::BlockHeader* response);
        bool GetUnclaimedTransactions(::UnclaimedTransactionList* response);
        bool GetUnclaimedTransactions(const std::string& user, ::UnclaimedTransactionList* response);
        bool Spend(const uint256_t& hash, const std::string& recipient);
    };
}

#endif //TOKEN_CLIENT_H