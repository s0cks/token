#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include <grpc++/grpc++.h>
#include "service.h"

namespace Token{
    class TokenServiceClient{
    private:
        std::shared_ptr<grpc::Channel> channel_;
        std::unique_ptr<Messages::TokenService::Stub> stub_;

        inline Messages::TokenService::Stub*
        GetStub(){
            return stub_.get();
        }
    public:
        TokenServiceClient(const std::string& address, int port);
        ~TokenServiceClient(){}

        bool GetHead(Messages::BlockHeader* response);
        bool GetBlock(const std::string& hash, Messages::BlockHeader* response);
        bool GetBlockAt(int index, Messages::BlockHeader* response);
        bool GetBlockData(const std::string& hash, Messages::BlockData* response);
        bool GetBlockData(int index, Messages::BlockData* response);
        bool GetUnclaimedTransactions(const std::string& user, Messages::UnclaimedTransactionsList* utxos);
        bool GetUnclaimedTransactions(Messages::UnclaimedTransactionsList* utxos);
        bool Append(Block* block, Messages::BlockHeader* response);
        bool Save();
    };
}

#endif //TOKEN_CLIENT_H