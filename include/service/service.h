#ifndef TOKEN_SERVICE_H
#define TOKEN_SERVICE_H

#include <grpc++/grpc++.h>
#include "service.pb.h"
#include "service.grpc.pb.h"

using namespace Token::Proto::BlockChain;
using namespace Token::Proto::BlockChainService;

namespace Token{
    class BlockChainService final : public ::BlockChainService::Service{
    private:
        std::unique_ptr<grpc::Server> server_;
    public:
        explicit BlockChainService():
                server_(){}
        ~BlockChainService(){}

        grpc::Status GetHead(grpc::ServerContext* ctx, const ::EmptyRequest* request, ::BlockHeader* response);
        grpc::Status GetBlock(grpc::ServerContext* ctx, const ::GetBlockRequest* request, ::BlockHeader* response);
        grpc::Status GetUnclaimedTransactions(grpc::ServerContext* ctx, const Proto::BlockChainService::GetUnclaimedTransactionsRequest* request, Proto::BlockChainService::UnclaimedTransactionList* response);
        grpc::Status Spend(grpc::ServerContext* ctx, ::UnclaimedTransaction* request, ::EmptyResponse* response);

        static BlockChainService* GetInstance();
        static void Start(const std::string address, int port);
        static void WaitForShutdown();
    };
}

#endif //TOKEN_SERVICE_H