#include "service/client.h"
#include <sstream>

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

    bool TokenServiceClient::GetBlockAt(int index, Token::Messages::BlockHeader *response){
        Token::Messages::Service::GetBlockRequest request;
        request.set_index(index);
        grpc::ClientContext ctx;
        return GetStub()->GetBlock(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlockData(const std::string &hash, Token::Messages::Block *response){
        Token::Messages::Service::GetBlockRequest request;
        request.set_hash(hash);
        grpc::ClientContext ctx;
        return GetStub()->GetBlockData(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetBlockDataAt(int index, Token::Messages::Block *response){
        Token::Messages::Service::GetBlockRequest request;
        request.set_index(index);
        grpc::ClientContext ctx;
        return GetStub()->GetBlockData(&ctx, request, response).ok();
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

    bool TokenServiceClient::GetPeers(std::vector<std::string> &peers){
        Token::Messages::EmptyRequest request;
        Token::Messages::PeerList response;
        grpc::ClientContext ctx;
        if(!GetStub()->GetPeers(&ctx, request, &response).ok()){
            return false;
        }
        for(auto& it : response.peers()){
            peers.push_back(it.address());
        }
        return true;
    }

    bool TokenServiceClient::ConnectTo(const std::string &address, int port){
        Token::Messages::Peer request;
        request.set_address(address);
        request.set_port(port);
        Token::Messages::EmptyResponse response;
        grpc::ClientContext ctx;
        return GetStub()->ConnectTo(&ctx, request, &response).ok();
    }

    bool TokenServiceClient::Spend(const std::string &token, const std::string &from_user,
                                   const std::string &to_user){
        Token::Messages::Service::SpendTokenRequest request;
        request.set_token(token);
        request.set_from_user(from_user);
        request.set_to_user(to_user);
        
        Token::Messages::EmptyResponse response;
        grpc::ClientContext ctx;
        return GetStub()->Spend(&ctx, request, &response).ok();
    }
}
