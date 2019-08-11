#include "service/service.h"

namespace Token{
    grpc::Status BlockChainService::GetHead(grpc::ServerContext *ctx,
                                            const Token::Service::Messages::EmptyRequest *request,
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
                                             const Token::Service::Messages::GetBlockRequest* request,
                                             Token::Messages::BlockHeader *response){
        if(!request->hash().empty()){
            Block* block = BlockChain::GetInstance()->GetBlockFromHash(request->hash());
            if(block){
                SetBlockHeader(block, response);
                return grpc::Status::OK;
            }
        } else{
            Block* block = BlockChain::GetInstance()->GetBlockAt(request->index());
            if(block){
                SetBlockHeader(block, response);
                return grpc::Status::OK;
            }
        }
        return grpc::Status::CANCELLED;
    }

    grpc::Status BlockChainService::GetBlockData(grpc::ServerContext *ctx,
                                                 const Token::Service::Messages::GetBlockRequest *request,
                                                 Token::Messages::Block *response){
        if(!request->hash().empty()){
            Block* block = BlockChain::GetInstance()->GetBlockFromHash(request->hash());
            if(block){
                Messages::Block* resp = block->GetAsMessage();
                response->CopyFrom(*resp);
                delete resp;
                return grpc::Status::OK;
            }
        } else{
            Block* block = BlockChain::GetInstance()->GetBlockAt(request->index());
            if(block){
                Messages::Block* resp = block->GetAsMessage();
                response->CopyFrom(*resp);
                delete resp;
                return grpc::Status::OK;
            }
        }
        return grpc::Status::CANCELLED;
    }

    grpc::Status BlockChainService::AppendBlock(grpc::ServerContext *ctx,
                                                const Messages::Block *request,
                                                Token::Messages::BlockHeader *response){
        Messages::Block bdata;
        bdata.CopyFrom(*request);
        Block* nblock = new Block(&bdata);
        std::cout << "Appending block w/ " << nblock->GetNumberOfTransactions() << " transactions" << std::endl;
        if(!BlockChain::GetInstance()->Append(nblock)){
            return grpc::Status::CANCELLED;
        }
        SetBlockHeader(nblock, response);
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::GetUnclaimedTransactions(grpc::ServerContext *ctx,
                                                             const Token::Service::Messages::GetUnclaimedTransactionsRequest *request,
                                                             Token::Messages::UnclaimedTransactionList *response){
        if(request->user().empty()){
            std::cout << "Getting unclaimed transactions for all" << std::endl;
            return UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransactions(response) ?
                   grpc::Status::OK :
                   grpc::Status::CANCELLED;
        } else{
            std::cout << "Getting unclaimed transactions for " << request->user() << std::endl;
            return UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransactions(request->user(), response) ?
                   grpc::Status::OK :
                   grpc::Status::CANCELLED;
        }
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