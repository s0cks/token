#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <grpc++/grpc++.h>
#include "service.pb.h"
#include "service.grpc.pb.h"

#include "block.h"

namespace Token{
    class BlockChainService final : public Token::Proto::BlockChainService::BlockChainService::Service{
    private:
        std::unique_ptr<grpc::Server> server_;
    public:
        explicit BlockChainService():
                server_(){}
        ~BlockChainService(){}

        grpc::Status GetHead(grpc::ServerContext* ctx, const Token::Proto::BlockChainService::EmptyRequest* request, BlockHeader::RawType* response);
        grpc::Status GetBlock(grpc::ServerContext* ctx, const Token::Proto::BlockChainService::GetBlockRequest* request, BlockHeader::RawType* response);
        grpc::Status GetUnclaimedTransactions(grpc::ServerContext* ctx, const Proto::BlockChainService::GetUnclaimedTransactionsRequest* request, Proto::BlockChainService::UnclaimedTransactionList* response);
        grpc::Status Spend(grpc::ServerContext* ctx, const Token::Proto::BlockChainService::SpendUnclaimedTransactionRequest* request, Token::Proto::BlockChainService::EmptyResponse* response);

        static BlockChainService* GetInstance();
        static void Start(const std::string address, int port);
        static void WaitForShutdown();
    };
}

#endif //TOKEN_SERVER_H