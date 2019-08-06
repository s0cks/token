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
        bool GetTransaction(const std::string& hash, int index, Transaction** response);
        bool GetTransactions(const std::string& hash, std::vector<Transaction*>& transactions);
        bool GetUnclaimedTransactions(const std::string& user, UnclaimedTransactionPool* utxos);
        bool GetUnclaimedTransactions(UnclaimedTransactionPool* utxos);
        bool Append(Block* block, Messages::BlockHeader* response);
        bool Save();

        bool GetCoinbase(const std::string& hash, Transaction** response){
            return GetTransaction(hash, 0, response);
        }
    };
}

#endif //TOKEN_CLIENT_H