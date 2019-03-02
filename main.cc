#include <grpc++/grpc++.h>
#include "service.grpc.pb.h"
#include "blockchain.h"
#include <memory.h>

class TokenServiceImpl final : public TokenService::Service{
private:
    std::string hash_;
    std::string merkle_;
    int height_;
public:
    explicit TokenServiceImpl(std::string hash, std::string merkle):
        merkle_(merkle),
        height_(0),
        hash_(hash){}

    grpc::Status List(grpc::ServerContext* context, const EmptyBlockHeader* request, BlockHeaderList* response){
        BlockHeader* header = response->add_blocks();
        header->set_hash(hash_);
        header->set_merkle_root(merkle_);
        header->set_height(height_++);
        return grpc::Status::OK;
    }
};

int main(int argc, char** argv){
    Token::Block* genesis = new Token::Block();
    Token::Transaction* tx = genesis->CreateTransaction();
    tx->AddOutput("User1", "Token1");
    tx->AddOutput("User1", "Token2");
    tx->AddOutput("User2", "Token3");

    TokenServiceImpl service(genesis->GetHash(), genesis->GetMerkleRoot());
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
    return 0;
}