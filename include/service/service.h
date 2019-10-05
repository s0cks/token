#ifndef TOKEN_SERVICE_H
#define TOKEN_SERVICE_H


#include <grpc++/grpc++.h>
#include "service.grpc.pb.h"
#include "blockchain.h"

namespace Token{
    class BlockChainService final : public Service::Messages::BlockChainService::Service{
    private:
        std::unique_ptr<grpc::Server> server_;

        inline void
        SetBlockHeader(Block* block, Messages::BlockHeader* header){
            header->set_previous_hash(block->GetPreviousHash());
            header->set_hash(block->GetHash());
            header->set_merkle_root(block->GetMerkleRoot());
            header->set_height(block->GetHeight());
        }

        inline void
        SetTransaction(Transaction* tx, Messages::Transaction* msg){
            msg->CopyFrom(*tx->GetRaw());
        }
    public:
        explicit BlockChainService():
                server_(){}
        ~BlockChainService(){}

        grpc::Status GetHead(grpc::ServerContext* ctx, const Token::Service::Messages::EmptyRequest* request, Messages::BlockHeader* response);
        grpc::Status GetBlock(grpc::ServerContext* ctx, const Token::Service::Messages::GetBlockRequest* request, Messages::BlockHeader* response);
        grpc::Status GetBlockData(grpc::ServerContext* ctx, const Token::Service::Messages::GetBlockRequest* request, Messages::Block* response);
        grpc::Status GetUnclaimedTransactions(grpc::ServerContext* ctx, const Token::Service::Messages::GetUnclaimedTransactionsRequest* request, Messages::UnclaimedTransactionList* response);
        grpc::Status GetPeers(grpc::ServerContext* ctx, const Token::Service::Messages::EmptyRequest* request, Token::Service::Messages::PeerList* response);
        grpc::Status Spend(grpc::ServerContext* ctx, const Token::Service::Messages::SpendTokenRequest* request, Token::Service::Messages::EmptyResponse* response);

        static BlockChainService* GetInstance();
        static void Start(const std::string address, int port);
        static void WaitForShutdown();
    };
}

#endif //TOKEN_SERVICE_H