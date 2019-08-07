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

        inline void
        SetBlockData(Block* block, Messages::BlockData* data){
            SetBlockHeader(block, data->mutable_header());
            for(int i = 0; i < block->GetNumberOfTransactions(); i++){
                SetTransaction(block->GetTransactionAt(i), data->add_transactions());
            }
        }
    public:
        explicit BlockChainService():
            server_(){}
        ~BlockChainService(){}

        grpc::Status GetHead(grpc::ServerContext* ctx, const Messages::EmptyRequest* request, Messages::BlockHeader* response);
        grpc::Status GetBlock(grpc::ServerContext* ctx, const Messages::GetBlockRequest* request, Messages::BlockHeader* response);
        grpc::Status GetBlockData(grpc::ServerContext* ctx, const Messages::GetBlockRequest* request, Messages::BlockData* response);
        grpc::Status GetUnclaimedTransactions(grpc::ServerContext* ctx, const Messages::GetUnclaimedTransactionsRequest* request, Messages::UnclaimedTransactionsList* response);
        grpc::Status AppendBlock(grpc::ServerContext* ctx, const Messages::Block* request, Messages::BlockHeader* response);
        grpc::Status Save(grpc::ServerContext* ctx, const Messages::EmptyRequest* request, Messages::EmptyResponse* response);

        static BlockChainService* GetInstance();
        static void Start(const std::string address, int port);
        static void WaitForShutdown();
    };
}

#endif //TOKEN_SERVICE_H