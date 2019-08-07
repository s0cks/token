#include "service/client.h"
#include <sstream>

namespace Token{
    TokenServiceClient::TokenServiceClient(const std::string &address, int port):
        stub_(),
        channel_(){
        std::stringstream ss;
        ss << address << ":" << port;
        channel_ = grpc::CreateChannel(ss.str(), grpc::InsecureChannelCredentials());
        stub_ = Messages::TokenService::NewStub(channel_);
    }

    bool TokenServiceClient::GetHead(Token::Messages::BlockHeader* response){
        Messages::EmptyRequest request;
        grpc::ClientContext ctx;
        return GetStub()->GetHead(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlock(const std::string &hash, Token::Messages::BlockHeader *response){
        Messages::GetBlockRequest request;
        request.set_hash(hash);
        grpc::ClientContext ctx;
        return GetStub()->GetBlock(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlockAt(int index, Token::Messages::BlockHeader *response){
        Messages::GetBlockRequest request;
        request.set_index(index);
        grpc::ClientContext ctx;
        return GetStub()->GetBlock(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlockData(const std::string &hash, Token::Messages::BlockData *response){
        Messages::GetBlockRequest request;
        request.set_hash(hash);
        grpc::ClientContext ctx;
        return GetStub()->GetBlockData(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlockData(int index, Messages::BlockData* response){
        Messages::GetBlockRequest request;
        request.set_index(index);
        grpc::ClientContext ctx;
        return GetStub()->GetBlockData(&ctx, request, response).ok();
    }

    bool TokenServiceClient::Append(Token::Block* block, Messages::BlockHeader* response){
        grpc::ClientContext ctx;
        Messages::Block request;
        request.CopyFrom(*block->GetRaw());
        return GetStub()->AppendBlock(&ctx, request, response).ok();
    }

    bool TokenServiceClient::Save(){
        grpc::ClientContext ctx;
        Messages::EmptyRequest request;
        Messages::EmptyResponse response;
        return GetStub()->Save(&ctx, request, &response).ok();
    }

    bool TokenServiceClient::GetUnclaimedTransactions(const std::string &user, Messages::UnclaimedTransactionsList* utxos){
        grpc::ClientContext ctx;
        Messages::GetUnclaimedTransactionsRequest request;
        request.set_user(user);
        Messages::UnclaimedTransactionsList response;
        return GetStub()->GetUnclaimedTransactions(&ctx, request, utxos).ok();
    }

    bool TokenServiceClient::GetUnclaimedTransactions(Messages::UnclaimedTransactionsList* utxos){
        grpc::ClientContext ctx;
        Messages::GetUnclaimedTransactionsRequest request;
        return GetStub()->GetUnclaimedTransactions(&ctx, request, utxos).ok();
    }
}