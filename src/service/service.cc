#include "service/service.h"
#include <glog/logging.h>

namespace Token{
    grpc::Status BlockChainService::GetHead(grpc::ServerContext *ctx,
                                            const Token::Messages::EmptyRequest *request,
                                            Token::Messages::BlockHeader *response){
        LOG(INFO) << "getting head";
        Block* head;
        if(!(head = BlockChain::GetHead())){
            LOG(ERROR) << "couldn't get BlockChain <HEAD>";
            return grpc::Status::CANCELLED;
        }
        SetBlockHeader(head, response);
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::GetBlock(grpc::ServerContext *ctx,
                                             const Token::Messages::Service::GetBlockRequest* request,
                                             Token::Messages::BlockHeader *response){
        if(!request->hash().empty()){
            LOG(INFO) << "getting block: " << request->hash();
            Block* block = BlockChain::GetBlock(request->hash());
            if(block){
                SetBlockHeader(block, response);
                return grpc::Status::OK;
            }
        } else{
            LOG(INFO) << "getting block @" << request->index();
            Block* block = BlockChain::GetBlock(request->index());
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
            Block* block = BlockChain::GetBlock(request->hash());
            if(block){
                //TODO Messages::Block* resp = block->GetAsMessage();
                // response->CopyFrom(*resp);
                // delete resp;
                return grpc::Status::OK;
            }
        } else{
            Block* block = BlockChain::GetBlock(request->index());
            if(block){
                //TODO Messages::Block* resp = block->GetAsMessage();
                // response->CopyFrom(*resp);
                // delete resp;
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

    grpc::Status BlockChainService::Spend(grpc::ServerContext *ctx,
                                          const Token::Messages::Service::SpendTokenRequest *request,
                                          Token::Messages::Transaction* response){
        LOG(INFO) << "spending " << request->token() << " to " << request->user();
        UnclaimedTransaction utxo;
        if(!UnclaimedTransactionPool::GetInstance()->GetUnclaimedTransaction(request->token(), &utxo)){
            LOG(ERROR) << "cannot get unclaimed transaction: " << request->token();
            return grpc::Status::CANCELLED;
        }


        LOG(INFO) << request->owner()<< " sent " << request->user() << " token: " << request->token();
        LOG(INFO) << "creating transaction...";

        Messages::Transaction raw;
        Messages::Input* in = raw.add_inputs();
        in->set_previous_hash(utxo.GetTransactionHash());
        in->set_index(utxo.GetIndex());

        Messages::Output* out = raw.add_outputs();
        out->set_token(utxo.GetToken());
        out->set_user(request->user());

        Transaction* tx = nullptr; //TODO: new Transaction(raw);
        TransactionSigner signer(tx);
        if(!signer.Sign()){
            LOG(ERROR) << "didn't sign transaction";
            return grpc::Status::CANCELLED;
        }
        if(!TransactionPool::AddTransaction(tx)) {
            LOG(ERROR) << "cannot add transaction to transaction pool";
            return grpc::Status::CANCELLED;
        }
        //TODO tx->Encode(response);
        delete tx;
        LOG(INFO) << "spent!";
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