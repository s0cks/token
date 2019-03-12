#include "service/service.h"


int main(int argc, char** argv){
    Token::TokenServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
    return 0;
}