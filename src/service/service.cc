#include "service/service.h"

using namespace Token::Messages;

namespace Token{
    grpc::Status
    TokenServiceImpl::GetHead(grpc::ServerContext *ctx, const ::GetHeadRequest* req, ::BlockHeader* resp){
        SetHeader(resp, GetHeadBlock());
        return grpc::Status::OK;
    }
}