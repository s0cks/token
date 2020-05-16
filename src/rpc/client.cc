#include <sstream>
#include <glog/logging.h>
#include "rpc/client.h"

namespace Token{
    TokenServiceClient::TokenServiceClient(const std::string &address, int port):
            stub_(),
            channel_(){
        std::stringstream ss;
        ss << address << ":" << port;
        channel_ = grpc::CreateChannel(ss.str(), grpc::InsecureChannelCredentials());
        stub_ = Token::Proto::BlockChainService::BlockChainService::NewStub(channel_);
    }

    TokenServiceClient::TokenServiceClient(const std::string &address):
            stub_(),
            channel_(){
        channel_ = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        stub_ = Token::Proto::BlockChainService::BlockChainService::NewStub(channel_);
    }

    bool TokenServiceClient::GetHead(BlockHeader* response){
        BlockHeader::RawType resp;
        Token::Proto::BlockChainService::EmptyRequest request;
        grpc::ClientContext ctx;
        if(!GetStub()->GetHead(&ctx, request, &resp).ok()) return false;
        (*response) = BlockHeader(resp);
        return true;
    }

    bool TokenServiceClient::GetBlock(const std::string &hash, BlockHeader* response){
        BlockHeader::RawType resp;
        Token::Proto::BlockChainService::GetBlockRequest request;
        request.set_hash(hash);
        grpc::ClientContext ctx;
        if(!GetStub()->GetBlock(&ctx, request, &resp).ok()) return false;
        (*response) = BlockHeader(resp);
        return true;
    }

    bool TokenServiceClient::GetUnclaimedTransactions(Token::Proto::BlockChainService::UnclaimedTransactionList *response){
        Token::Proto::BlockChainService::GetUnclaimedTransactionsRequest request;
        grpc::ClientContext ctx;
        return GetStub()->GetUnclaimedTransactions(&ctx, request, response).ok();
    }

    bool TokenServiceClient::GetUnclaimedTransactions(const std::string& user, Token::Proto::BlockChainService::UnclaimedTransactionList *response){
        Token::Proto::BlockChainService::GetUnclaimedTransactionsRequest request;
        request.set_user_id(user);

        grpc::ClientContext ctx;
        return GetStub()->GetUnclaimedTransactions(&ctx, request, response).ok();
    }

    bool TokenServiceClient::Spend(const uint256_t& hash, const std::string& dest, Transaction* response){
        Token::Proto::BlockChainService::SpendUnclaimedTransactionRequest request;
        request.set_user_id(dest);
        request.set_utxo(HexString(hash));

        Transaction::RawType resp;
        grpc::ClientContext ctx;
        if(!GetStub()->Spend(&ctx, request, &resp).ok()) return false;
        (*response) = Transaction(resp);
        return true;
    }
}
