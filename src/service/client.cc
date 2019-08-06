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
        Messages::BlockHash bhash;
        bhash.set_hash(hash);
        grpc::ClientContext ctx;
        return GetStub()->GetBlock(&ctx, bhash, response).ok();
    }

    bool TokenServiceClient::GetBlockAt(int index, Token::Messages::BlockHeader *response){
        Messages::BlockIndex bindex;
        bindex.set_index(index);
        grpc::ClientContext ctx;
        return GetStub()->GetBlockAt(&ctx, bindex, response).ok();
    }

    bool TokenServiceClient::GetTransaction(const std::string &hash, int index, Transaction** response){
        Messages::GetTransactionRequest request;
        request.set_hash(hash);
        request.set_index(static_cast<google::protobuf::uint32>(index));

        Messages::Transaction* raw = new Messages::Transaction();
        grpc::ClientContext ctx;
        grpc::Status resp = GetStub()->GetTransaction(&ctx, request, raw);
        if(resp.ok()){
            *response = new Transaction(raw);
        } else{
            *response = nullptr;
        }
        return resp.ok();
    }

    bool TokenServiceClient::GetTransactions(const std::string &hash, std::vector<Token::Transaction*>& transactions){
        Messages::BlockHash bhash;
        bhash.set_hash(hash);

        Messages::BlockData data;
        grpc::ClientContext ctx;
        if(!GetStub()->GetTransactions(&ctx, bhash, &data).ok()){
            return false;
        }
        std::cout << "Collected " << data.transactions_size() << " transactions" << std::endl;
        for(int i = 0; i < data.transactions_size(); i++){
            Messages::Transaction* tx = new Messages::Transaction();
            tx->CopyFrom(data.transactions(i));
            transactions.push_back(new Transaction(tx));
        }
        return true;
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

    bool TokenServiceClient::GetUnclaimedTransactions(const std::string &user, UnclaimedTransactionPool* utxos){
        grpc::ClientContext ctx;
        Messages::GetUnclaimedTransactionsRequest request;
        request.set_user(user);
        Messages::UnclaimedTransactionsList response;
        if(!GetStub()->GetUnclaimedTransactions(&ctx, request, &response).ok()) {
            utxos->Clear();
            return false;
        }
        utxos->Clear();
        for(int i = 0; i < response.transactions_size(); i++){
            Messages::UnclaimedTransaction utxo = response.transactions(i);
            utxos->Insert(utxo.hash(), utxo.index(), new Output(utxo.user(), utxo.token()));
        }
        return true;
    }

    bool TokenServiceClient::GetUnclaimedTransactions(UnclaimedTransactionPool* utxos){
        grpc::ClientContext ctx;
        Messages::GetUnclaimedTransactionsRequest request;
        Messages::UnclaimedTransactionsList response;
        if(!GetStub()->GetUnclaimedTransactions(&ctx, request, &response).ok()){
            utxos->Clear();
            return false;
        }
        utxos->Clear();
        for(int i = 0; i < response.transactions().size(); i++){
            Messages::UnclaimedTransaction utxo = response.transactions(i);
            utxos->Insert(utxo.hash(), utxo.index(), new Output(utxo.user(), utxo.token()));
        }
        return true;
    }
}