
#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include <grpc++/grpc++.h>
#include "server.h"
#include "block.h"
#include "block_chain.h"

namespace Token{
    class TokenServiceClient{
    private:
        std::shared_ptr<grpc::Channel> channel_;
        std::unique_ptr<Token::Proto::BlockChainService::BlockChainService::Stub> stub_;

        inline Token::Proto::BlockChainService::BlockChainService::Stub*
        GetStub(){
            return stub_.get();
        }
    public:
        TokenServiceClient(const std::string& address, int port);
        TokenServiceClient(const std::string& address);
        ~TokenServiceClient(){}

        bool GetHead(BlockHeader::RawType* response);
        bool GetBlock(const std::string& hash, BlockHeader::RawType* response);
        bool GetUnclaimedTransactions(Token::Proto::BlockChainService::UnclaimedTransactionList* response);
        bool GetUnclaimedTransactions(const std::string& user, Token::Proto::BlockChainService::UnclaimedTransactionList* response);
        bool Spend(const uint256_t& hash, const std::string& recipient);
    };
}

#endif //TOKEN_CLIENT_H