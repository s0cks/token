#include <grpc++/grpc++.h>
#include "service.grpc.pb.h"

class TokenServiceClient{
private:
    std::unique_ptr<TokenService::Stub> stub_;
public:
    TokenServiceClient(std::shared_ptr<grpc::Channel> channel):
        stub_(TokenService::NewStub(channel)){}
        
    void ListBlocks(){
        EmptyBlockHeader header;
        
        BlockHeaderList list;
        grpc::ClientContext ctx;
        stub_->List(&ctx, header, &list);
        
        for(int i = 0; i < list.blocks_size(); i++){
                BlockHeader blockHeader = list.blocks(i);
                std::cout << blockHeader.height() << std::endl;
                std::cout << blockHeader.merkle_root() << std::endl;
                std::cout << blockHeader.hash() << std::endl;
        }
    }
};

int main(int argc, char** argv){
        TokenServiceClient client(grpc::CreateChannel(
                        "localhost:50051",
                        grpc::InsecureChannelCredentials()
                ));
        client.ListBlocks();
        return 0;
}