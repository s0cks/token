#include "service/service.h"
#include <glog/logging.h>

namespace Token{
    grpc::Status BlockChainService::GetHead(grpc::ServerContext *ctx,
                                            const Token::Messages::EmptyRequest *request,
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
                                             const Token::Messages::Service::GetBlockRequest* request,
                                             Token::Messages::BlockHeader *response){
        if(!request->hash().empty()){
            LOG(INFO) << "getting block: " << request->hash();
            Block* block = BlockChain::GetInstance()->GetBlock(request->hash());
            if(block){
                SetBlockHeader(block, response);
                return grpc::Status::OK;
            }
        } else{
            LOG(INFO) << "getting block @" << request->index();
            Block* block = BlockChain::GetInstance()->GetBlock(request->index());
            if(block){
                SetBlockHeader(block, response);
                return grpc::Status::OK;
            }
        }
        return grpc::Status::CANCELLED;
    }

    grpc::Status BlockChainService::GetBlockData(grpc::ServerContext *ctx,
                                                 const Token::Messages::Service::GetBlockRequest *request,
                                                 Token::Messages::Block *response){
        if(!request->hash().empty()){
            Block* block = BlockChain::GetInstance()->GetBlock(request->hash());
            if(block){
                Messages::Block* resp = block->GetAsMessage();
                response->CopyFrom(*resp);
                delete resp;
                return grpc::Status::OK;
            }
        } else{
            Block* block = BlockChain::GetInstance()->GetBlock(request->index());
            if(block){
                Messages::Block* resp = block->GetAsMessage();
                response->CopyFrom(*resp);
                delete resp;
                return grpc::Status::OK;
            }
        }
        return grpc::Status::CANCELLED;
    }

    grpc::Status BlockChainService::GetUnclaimedTransactions(grpc::ServerContext *ctx,
                                                             const Token::Messages::Service::GetUnclaimedTransactionsRequest *request,
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
                                             const Token::Messages::EmptyRequest *request,
                                             Token::Messages::PeerList *response){
        /*
         * TODO:
         * if(!BlockChainServer::GetPeerList(*response)){
         *      LOG(ERROR) << "couldn't get peer list";
         *      return grpc::Status::CANCELLED;
         * }
         */
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::Spend(grpc::ServerContext *ctx,
                                          const Token::Messages::Service::SpendTokenRequest *request,
                                          Token::Messages::EmptyResponse* response){
        LOG(INFO) << "spending: " << request->token();
        UnclaimedTransaction utxo;
        if(!UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransaction(request->token(), &utxo)){
            LOG(ERROR) << "cannot get unclaimed transaction: " << request->token();
            return grpc::Status::CANCELLED;
        }

        LOG(INFO) << request->from_user() << " sent " << request->to_user() << " token: " << request->token();
        LOG(INFO) << "creating transaction...";
        Transaction* tx = new Transaction();
        tx->AddInput(utxo.GetTransactionHash(), utxo.GetIndex());
        tx->AddOutput(request->to_user(), utxo.GetToken());
        if(!TransactionPool::AddTransaction(tx)){
            LOG(ERROR) << "cannot add transaction to transaction pool";
            return grpc::Status::CANCELLED;
        }
        return grpc::Status::OK;
    }

    /// 5O57b4u7f3uteTEUiEY1
    
    grpc::Status BlockChainService::ConnectTo(grpc::ServerContext *ctx, const Token::Messages::Peer *request,
                                              Token::Messages::EmptyResponse *response){
        /*
         * if(!BlockChainServer::ConnectToPeer(request->address(), request->port())){
         *     LOG(ERROR) << "couldn't connect to peer";
         *     return grpc::Status::CANCELLED;
         * }
         */
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