#include "service/service.h"
#include <glog/logging.h>
#include <server.h>

namespace Token{
    grpc::Status BlockChainService::GetHead(grpc::ServerContext *ctx,
                                            const Token::Service::Messages::EmptyRequest *request,
                                            Token::Messages::BlockHeader *response){
        LOG(INFO) << "getting head";
        Block* head;
        if(!(head = BlockChain::GetInstance()->GetHead())){
            LOG(ERROR) << "couldn't get BlockChain <HEAD>";
            return grpc::Status::CANCELLED;
        }
        LOG(INFO) << "<HEAD>:";
        LOG(INFO) << (*head);
        SetBlockHeader(head, response);
        return grpc::Status::OK;
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
        //TODO: Refactor this copy
        Messages::Block bdata;
        bdata.CopyFrom(*request);
        Block* nblock = new Block(&bdata);
        LOG(INFO) << "appending new block: " << nblock->GetHash();
        if(!BlockChain::GetInstance()->Append(nblock)){
            LOG(ERROR) << "couldn't append new block";
            return grpc::Status::CANCELLED;
        }

        LOG(WARNING) << "appended, broadcasting...";
        Message msg(Message::Type::kBlockMessage, nblock->GetRaw());
        if(!BlockChainServer::Broadcast(nullptr, &msg)){
            LOG(ERROR) << "couldn't broadcast to peers";
            return grpc::Status::CANCELLED;
        }
        LOG(INFO) << "done!";
        SetBlockHeader(nblock, response);
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::GetUnclaimedTransactions(grpc::ServerContext *ctx,
                                                             const Token::Service::Messages::GetUnclaimedTransactionsRequest *request,
                                                             Token::Messages::UnclaimedTransactionList *response){
        if(request->user().empty()){
            return UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransactions(response) ?
                   grpc::Status::OK :
                   grpc::Status::CANCELLED;
        } else{
            return UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransactions(request->user(), response) ?
                   grpc::Status::OK :
                   grpc::Status::CANCELLED;
        }
    }

    grpc::Status BlockChainService::GetPeers(grpc::ServerContext *ctx,
                                             const Token::Service::Messages::EmptyRequest *request,
                                             Token::Messages::PeerList *response){
        std::vector<std::string> peers;
        if(!BlockChainServer::GetPeers(peers)){
            LOG(ERROR) << "couldn't get peers";
            return grpc::Status::CANCELLED;
        }
        for(auto& it : peers){
            response->add_peers()->set_address(it);
        }
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
    }

    void BlockChainService::WaitForShutdown(){
        GetInstance()->server_->Wait();
    }
}