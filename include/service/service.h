#ifndef TOKEN_SERVICE_H
#define TOKEN_SERVICE_H

#include <grpc++/grpc++.h>
#include "service.grpc.pb.h"
#include "blockchain.h"

namespace Token{
    class BlockChainService final : public Service::BlockChainService::Service{
    private:
        std::unique_ptr<grpc::Server> server_;
    public:
        explicit BlockChainService():
            server_(){
        }
        ~BlockChainService(){}
    };
}

#endif //TOKEN_SERVICE_H