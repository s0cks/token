#ifndef TOKEN_SERVICE_H
#define TOKEN_SERVICE_H


#include <grpc++/grpc++.h>
#include "service.grpc.pb.h"
#include "blockchain.h"

namespace Token{
    class BlockChainService final : public Messages::Service::BlockChainService::Service{
    private:
        std::unique_ptr<grpc::Server> server_;

        inline void
        SetBlockHeader(Block* block, Messages::BlockHeader* header){
            header->set_previous_hash(block->GetPreviousHash());
            header->set_hash(block->GetHash());
            header->set_merkle_root(block->GetMerkleRoot());
            header->set_height(block->GetHeight());
        }
    public:
        explicit BlockChainService():
                server_(){}
        ~BlockChainService(){}

        grpc::Status GetHead(grpc::ServerContext* ctx, const Token::Messages::EmptyRequest* request, Messages::BlockHeader* response);
        grpc::Status GetBlock(grpc::ServerContext* ctx, const Token::Messages::Service::GetBlockRequest* request, Messages::BlockHeader* response);
        grpc::Status GetBlockData(grpc::ServerContext* ctx, const Token::Messages::Service::GetBlockRequest* request, Messages::Block* response);
        grpc::Status GetUnclaimedTransactions(grpc::ServerContext* ctx, const Token::Messages::Service::GetUnclaimedTransactionsRequest* request, Messages::UnclaimedTransactionList* response);
        grpc::Status SpendToken(grpc::ServerContext* ctx, const Token::Messages::Service::SpendTokenRequest* request, Messages::Transaction* response);

        static BlockChainService* GetInstance();
        static void Start(const std::string address, int port);
        static void WaitForShutdown();
    };
}

#endif //TOKEN_SERVICE_H