#include <sstream>
#include <glog/logging.h>
#include "service/client.h"

namespace Token{
    TokenServiceClient::TokenServiceClient(const std::string &address, int port):
            stub_(),
            channel_(){
        std::stringstream ss;
        ss << address << ":" << port;
        channel_ = grpc::CreateChannel(ss.str(), grpc::InsecureChannelCredentials());
        stub_ = ::BlockChainService::NewStub(channel_);
    }

    TokenServiceClient::TokenServiceClient(const std::string &address):
            stub_(),
            channel_(){
        channel_ = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        stub_ = ::BlockChainService::NewStub(channel_);
    }

    bool TokenServiceClient::GetHead(::BlockHeader* response){
        ::EmptyRequest request;
        grpc::ClientContext ctx;
        return GetStub()->GetHead(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlock(const std::string &hash, ::BlockHeader *response){
        ::GetBlockRequest request;
        request.set_hash(hash);
        grpc::ClientContext ctx;
        return GetStub()->GetBlock(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetUnclaimedTransactions(::UnclaimedTransactionList *response){
        ::GetUnclaimedTransactionsRequest request;
        grpc::ClientContext ctx;
        return GetStub()->GetUnclaimedTransactions(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetUnclaimedTransactions(const std::string& user, ::UnclaimedTransactionList *response){
        ::GetUnclaimedTransactionsRequest request;
        request.set_user_id(user);

        grpc::ClientContext ctx;
        return GetStub()->GetUnclaimedTransactions(&ctx, request, response).ok();
    }

    bool TokenServiceClient::Spend(Token::UnclaimedTransaction* utxo){
        ::UnclaimedTransaction request;
        request << (*utxo);

        ::EmptyResponse response;

        grpc::ClientContext ctx;
        return GetStub()->Spend(&ctx, request, &response).ok();
    }
}
