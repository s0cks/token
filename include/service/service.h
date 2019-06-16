#ifndef TOKEN_SERVICE_H
#define TOKEN_SERVICE_H

#include <grpc++/grpc++.h>
#include "service.grpc.pb.h"
#include "blockchain.h"

namespace Token{
    class BlockChainService final : public Messages::TokenService::Service{
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

        grpc::Status GetHead(grpc::ServerContext* ctx, const Messages::EmptyRequest* request, Messages::BlockHeader* response);
        grpc::Status GetBlock(grpc::ServerContext* ctx, const Messages::BlockHash* request, Messages::BlockHeader* response);
        grpc::Status GetBlockAt(grpc::ServerContext* ctx, const Messages::BlockIndex* request, Messages::BlockHeader* response);
        grpc::Status GetTransaction(grpc::ServerContext* ctx, const Messages::GetTransactionRequest* request, Messages::Transaction* response);
        grpc::Status GetTransactions(grpc::ServerContext* ctx, const Messages::BlockHash* request, Messages::BlockData* response);
        grpc::Status AppendBlock(grpc::ServerContext* ctx, const Messages::Block* request, Messages::BlockHeader* response);

        static BlockChainService* GetInstance();
        static void Start(const std::string address, int port);
        static void WaitForShutdown();
    };
}

#endif //TOKEN_SERVICE_H