#include "client/client.h"

static void
DumpBlock(Token::Messages::BlockHeader* block){
    std::cout << "Block #" << block->height() << std::endl;
    std::cout << "\t+ Block Hash := " << block->hash() << std::endl;
    std::cout << "\t+ Block Merkle Root := " << block->merkle_root() << std::endl;
}

int main(int argc, char** argv){
        Token::TokenServiceClient client(grpc::CreateChannel(
                        "localhost:50051",
                        grpc::InsecureChannelCredentials()
                ));

        Token::Messages::BlockHeader head;
        client.GetHead(&head);

        std::cout << "<HEAD>" << std::endl;
        DumpBlock(&head);
        return 0;
}