#ifdef TOKEN_ENABLE_REST_SERVICE

#include "blockchain.h"
#include "http/http_service.h"
#include "http/http_controller_chain.h"

namespace token{
#define DEFINE_ENDPOINT_HANDLER(Method, Path, Name) \
  HTTP_CONTROLLER_ROUTE_HANDLER(ChainController, Name);

  FOR_EACH_CHAIN_CONTROLLER_ENDPOINT(DEFINE_ENDPOINT_HANDLER)
#undef DEFINE_ENDPOINT_HANDLER

  HTTP_CONTROLLER_ENDPOINT_HANDLER(ChainController, GetBlockChain){
    BlockChain* chain = GetChain();

    Json::String body;
    Json::Writer writer(body);
    if(!chain->GetBlocks(writer))
      return session->Send(NewInternalServerErrorResponse(session, "Cannot list block chain blocks."));
    return session->Send(NewOkResponse(session, body));
  }

  HTTP_CONTROLLER_ENDPOINT_HANDLER(ChainController, GetBlockChainHead){
    BlockChain* chain = GetChain();
    BlockPtr head = chain->GetHead();
    HttpResponsePtr response = NewOkResponse(session, head);
    return session->Send(response);
  }

  HTTP_CONTROLLER_ENDPOINT_HANDLER(ChainController, GetBlockChainBlock){
    BlockChain* chain = GetChain();

    Hash hash = request->GetHashParameterValue();
    if(!chain->HasBlock(hash))
      return session->Send(NewNoContentResponse(session, hash));

    BlockPtr blk = chain->GetBlock(hash);
    return session->Send(NewOkResponse(session, blk));
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE