#include <glog/logging.h>
#include "service/service.h"
#include "block_chain.h"

namespace Token{
    static inline void
    SetBlockHeader(::BlockHeader* header, Block* block){
        header->set_timestamp(block->GetTimestamp());
        header->set_height(block->GetHeight());
        header->set_previous_hash(HexString(block->GetPreviousHash()));
        header->set_merkle_root(HexString(block->GetMerkleRoot()));
        header->set_hash(HexString(block->GetHash()));
        header->set_num_transactions(block->GetNumberOfTransactions());
    }

    grpc::Status BlockChainService::GetHead(grpc::ServerContext *ctx, const ::EmptyRequest *request, ::BlockHeader *response) {
        Block* head;
        if(!(head = BlockChain::GetBlock(BlockChain::GetHead()))){
            LOG(WARNING) << "couldn't get block chain <HEAD>";
            return grpc::Status::CANCELLED;
        }
        LOG(INFO) << "getting <HEAD>";
        SetBlockHeader(response, head);
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::GetBlock(grpc::ServerContext *ctx, const ::GetBlockRequest *request, ::BlockHeader *response){
        if(request->hash().empty()){
            LOG(WARNING) << "requested block hash is empty";
            return grpc::Status::CANCELLED;
        } else if(request->hash().length() != 64){
            LOG(WARNING) << "requested block hash " << request->hash() << " is invalid";
            return grpc::Status::CANCELLED;
        }
        Block* result;
        if(!(result = BlockChain::GetBlock(HashFromHexString(request->hash())))){
            LOG(WARNING) << "couldn't find block: " << request->hash();
            return grpc::Status::CANCELLED;
        }
        SetBlockHeader(response, result);
        delete result;
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::GetUnclaimedTransactions(grpc::ServerContext *ctx, const Proto::BlockChainService::GetUnclaimedTransactionsRequest* request, Proto::BlockChainService::UnclaimedTransactionList* response){
        if(request->user_id().empty()){
            // get all unclaimed transactions
            LOG(INFO) << "getting all unclaimed transactions";
            //return UnclaimedTransactionPool::GetUnclaimedTransactions(response) ?
            // grpc::Status::OK :
            // grpc::Status::CANCELLED;
        }
        // get all unclaimed transactions for user_id
        LOG(INFO) << "getting all unclaimed transactions for: " << request->user_id();
        //return UnclaimedTransactionPool::GetUnclaimedTransactions(request->user_id(), response) ?
        //    grpc::Status::OK :
        //    grpc::Status::CANCELLED;
        return grpc::Status::CANCELLED;
    }

    grpc::Status BlockChainService::Spend(grpc::ServerContext *ctx, ::UnclaimedTransaction *request, ::EmptyResponse *response){
        LOG(WARNING) << "Spend() not implemented!";
        return grpc::Status::CANCELLED;
    }

    BlockChainService* BlockChainService::GetInstance(){
        static BlockChainService instance;
        return &instance;
    }

    void BlockChainService::Start(const std::string address, int port){
        BlockChainService* instance = GetInstance();
        std::stringstream ss;
        ss << address << ":" << port;
        grpc::ServerBuilder builder;
        builder.AddListeningPort(ss.str(), grpc::InsecureServerCredentials());
        builder.RegisterService(instance);
        instance->server_ = std::unique_ptr<grpc::Server>(builder.BuildAndStart());
    }

    void BlockChainService::WaitForShutdown(){
        GetInstance()->server_->Wait();
    }
}