#include "service/service.h"

namespace Token{
    grpc::Status BlockChainService::GetHead(grpc::ServerContext *ctx,
                                            const Token::Messages::EmptyRequest *request,
                                            Token::Messages::BlockHeader *response){
        std::cout << "Processing GetHead..." << std::endl;
        Block* head = BlockChain::GetInstance()->GetHead();
        if(head){
            SetBlockHeader(head, response);
            return grpc::Status::OK;
        }
        return grpc::Status::CANCELLED;
    }

    grpc::Status BlockChainService::GetBlock(grpc::ServerContext *ctx,
                                            const Token::Messages::BlockHash* request,
                                             Token::Messages::BlockHeader *response){
        Block* block = BlockChain::GetInstance()->GetBlockFromHash(request->hash());
        if(block){
            SetBlockHeader(block, response);
            return grpc::Status::OK;
        }
        return grpc::Status::CANCELLED;
    }

    grpc::Status BlockChainService::GetBlockAt(grpc::ServerContext *ctx,
                                                const Messages::BlockIndex* request,
                                               Token::Messages::BlockHeader *response){
        Block* block = BlockChain::GetInstance()->GetBlockAt(request->index());
        if(block){
            SetBlockHeader(block, response);
            return grpc::Status::OK;
        }
        return grpc::Status::CANCELLED;
    }

    grpc::Status BlockChainService::GetTransaction(grpc::ServerContext *ctx,
                                                   const Messages::GetTransactionRequest *request,
                                                   Token::Messages::Transaction *response){
        Block* block = BlockChain::GetInstance()->GetBlockFromHash(request->hash());
        if(!block || request->index() > block->GetNumberOfTransactions() - 1){
            return grpc::Status::CANCELLED;
        }
        SetTransaction(block->GetTransactionAt(request->index()), response);
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::AppendBlock(grpc::ServerContext *ctx,
                                                const Messages::Block *request,
                                                Token::Messages::BlockHeader *response){
        Messages::Block* bdata = new Messages::Block();
        bdata->CopyFrom(*request);
        Block* nblock = new Block(bdata);
        std::cout << "Append block w/ " << nblock->GetNumberOfTransactions() << " transactions" << std::endl;
        if(!BlockChain::GetInstance()->Append(nblock)){
            return grpc::Status::CANCELLED;
        }
        SetBlockHeader(BlockChain::GetInstance()->GetHead(), response);
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::GetTransactions(grpc::ServerContext *ctx,
                                                    const Token::Messages::BlockHash *request,
                                                    Messages::BlockData* response){
        Block* block = BlockChain::GetInstance()->GetBlockFromHash(request->hash());
        if(!block || block->GetNumberOfTransactions() == 0){
            return grpc::Status::CANCELLED;
        }
        response->mutable_transactions()->CopyFrom(block->GetRaw()->transactions());
        return grpc::Status::OK;
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
        std::cout << "Started Service: " << ss.str() << std::endl;
    }

    void BlockChainService::WaitForShutdown(){
        GetInstance()->server_->Wait();
    }
}