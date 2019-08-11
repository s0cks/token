#include "service/client.h"
#include <sstream>

namespace Token{
    TokenServiceClient::TokenServiceClient(const std::string &address, int port):
            stub_(),
            channel_(){
        std::stringstream ss;
        ss << address << ":" << port;
        channel_ = grpc::CreateChannel(ss.str(), grpc::InsecureChannelCredentials());
        stub_ = Token::Service::BlockChainService::NewStub(channel_);
    }

    bool TokenServiceClient::GetHead(Token::Messages::BlockHeader* response){
        Token::Service::Messages::EmptyRequest request;
        grpc::ClientContext ctx;
        return GetStub()->GetHead(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlock(const std::string &hash, Token::Messages::BlockHeader *response){
        Token::Service::Messages::GetBlockRequest request;
        request.set_hash(hash);
        grpc::ClientContext ctx;
        return GetStub()->GetBlock(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlockAt(int index, Token::Messages::BlockHeader *response){
        Token::Service::Messages::GetBlockRequest request;
        request.set_index(index);
        grpc::ClientContext ctx;
        return GetStub()->GetBlock(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlockData(const std::string &hash, Token::Messages::Block *response){
        Token::Service::Messages::GetBlockRequest request;
        request.set_hash(hash);
        grpc::ClientContext ctx;
        return GetStub()->GetBlockData(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlockDataAt(int index, Token::Messages::Block *response){
        Token::Service::Messages::GetBlockRequest request;
        request.set_index(index);
        grpc::ClientContext ctx;
        return GetStub()->GetBlockData(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetUnclaimedTransactions(Token::Messages::UnclaimedTransactionList *response){
        Token::Service::Messages::GetUnclaimedTransactionsRequest request;
        grpc::ClientContext ctx;
        return GetStub()->GetUnclaimedTransactions(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetUnclaimedTransactions(const std::string& user, Token::Messages::UnclaimedTransactionList *response){
        Token::Service::Messages::GetUnclaimedTransactionsRequest request;
        grpc::ClientContext ctx;
        return GetStub()->GetUnclaimedTransactions(&ctx, request, response).ok();
    }

    bool TokenServiceClient::Append(Token::Block* block, Messages::BlockHeader* response){
        grpc::ClientContext ctx;
        Token::Messages::Block* blk = block->GetAsMessage();
        bool status = GetStub()->AppendBlock(&ctx, *blk, response).ok();
        delete blk;
        return status;
    }
}
