#include <glog/logging.h>
#include "service/service.h"
#include "block_chain.h"

namespace Token{
    grpc::Status BlockChainService::GetHead(grpc::ServerContext *ctx, const ::EmptyRequest *request, ::BlockHeader *response) {
        (*response) << BlockChain::GetHead();
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::GetBlock(grpc::ServerContext *ctx, const ::GetBlockRequest *request, ::BlockHeader *response){
        if(request->hash().empty()){
            LOG(WARNING) << "requested block hash is empty";
            return grpc::Status::CANCELLED;
        } else if(request->hash().length() != 64){
            LOG(WARNING) << "requested block hash " << request->hash() << " is invalid";
            return grpc::Status::CANCELLED;
        }

        uint256_t hash = HashFromHexString(request->hash());
        if(!BlockChain::ContainsBlock(hash)){
            LOG(WARNING) << "couldn't find requested block: " << hash;
            return grpc::Status::CANCELLED;
        }
        (*response) << BlockChain::GetBlock(hash);
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::GetUnclaimedTransactions(grpc::ServerContext *ctx, const Proto::BlockChainService::GetUnclaimedTransactionsRequest* request, Proto::BlockChainService::UnclaimedTransactionList* response){
        std::string user = request->user_id();

        std::vector<uint256_t> utxos;
        if(user.empty()){
            // get all unclaimed transactions
            if(!UnclaimedTransactionPool::GetUnclaimedTransactions(utxos)){
                LOG(WARNING) << "couldn't get all unclaimed transactions";
                return grpc::Status::CANCELLED;
            }
        } else{
            // get all unclaimed transactions for user_id
            if(!UnclaimedTransactionPool::GetUnclaimedTransactions(user, utxos)){
                LOG(WARNING) << "couldn't get all unclaimed transactions for: " << user;
                return grpc::Status::CANCELLED;
            }
        }
        for(auto& it : utxos) response->add_utxos(HexString(it));
        return grpc::Status::OK;
    }

    grpc::Status BlockChainService::Spend(grpc::ServerContext* ctx, const ::SpendUnclaimedTransactionRequest* request, ::EmptyResponse* response){
        uint256_t hash = HashFromHexString(request->utxo());

        UnclaimedTransaction utxo;
        if(!UnclaimedTransactionPool::GetUnclaimedTransaction(hash, &utxo)){
            LOG(ERROR) << "couldn't get unclaimed transaction: " << hash;
            return grpc::Status::CANCELLED;
        }

        Transaction tx(0);
        tx << Input(utxo);
        tx << Output(request->user_id(), "TestToken");

        uint256_t tx_hash = tx.GetHash();
        if(!TransactionPool::PutTransaction(&tx)){
            LOG(ERROR) << "couldn't put transaction: " << tx_hash;
            return grpc::Status::CANCELLED;
        }

        LOG(INFO) << "spent: " << tx_hash;
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