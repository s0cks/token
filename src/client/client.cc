#include "client/client.h"

namespace Token{
    void
    TokenServiceClient::GetHead(::BlockHeader* block){
        ::GetHeadRequest request;
        grpc::ClientContext ctx;
        stub_->GetHead(&ctx, request, block);
    }
}