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
        stub_ = Token::Messages::Service::BlockChainService::NewStub(channel_);
    }

    TokenServiceClient::TokenServiceClient(const std::string &address):
            stub_(),
            channel_(){
        channel_ = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        stub_ = Token::Messages::Service::BlockChainService::NewStub(channel_);
    }

    bool TokenServiceClient::GetHead(Token::Messages::BlockHeader* response){
        Token::Messages::EmptyRequest request;
        grpc::ClientContext ctx;
        return GetStub()->GetHead(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlock(const std::string &hash, Token::Messages::BlockHeader *response){
        Token::Messages::Service::GetBlockRequest request;
        request.set_hash(hash);
        grpc::ClientContext ctx;
        return GetStub()->GetBlock(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetUnclaimedTransactions(Token::Messages::UnclaimedTransactionList *response){
        Token::Messages::Service::GetUnclaimedTransactionsRequest request;
        grpc::ClientContext ctx;
        return GetStub()->GetUnclaimedTransactions(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetUnclaimedTransactions(const std::string& user, Token::Messages::UnclaimedTransactionList *response){
        Token::Messages::Service::GetUnclaimedTransactionsRequest request;
        request.set_user(user);
        grpc::ClientContext ctx;
        return GetStub()->GetUnclaimedTransactions(&ctx, request, response).ok();
    }

    bool TokenServiceClient::SpendToken(const std::string &token, const std::string &from_user,
                                   const std::string &to_user, Transaction** tx){
        Token::Messages::Service::SpendTokenRequest request;
        request.set_token(token);
        request.set_user(to_user);
        request.set_owner(from_user);

        grpc::ClientContext ctx;
        Messages::Transaction response;
        if(!GetStub()->Spend(&ctx, request, &response).ok()) {
            (*tx) = nullptr;
            return false;
        }
        //TODO: (*tx) = new Transaction(response);
        return true;
    }
}
