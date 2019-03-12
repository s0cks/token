#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include <grpc++/grpc++.h>
#include "service.grpc.pb.h"

using namespace Token::Messages;

namespace Token{
    class TokenServiceClient{
    private:
        std::unique_ptr<::TokenService::Stub> stub_;
    public:
        TokenServiceClient(std::shared_ptr<grpc::Channel> channel):
            stub_(::TokenService::NewStub(channel)){}
        ~TokenServiceClient(){}

        void GetHead(::BlockHeader* block);
    };
}

#endif //TOKEN_CLIENT_H