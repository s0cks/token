#ifdef TOKEN_ENABLE_REST_SERVICE

#include "blockchain.h"
#include "http/http_service.h"
#include "http/http_controller_chain.h"

namespace token{
  void ChainController::HandleGetBlockChain(HttpSession* session, const HttpRequestPtr& request){
    BlockChain* chain = BlockChain::GetInstance();

    Json::String body;
    Json::Writer writer(body);
    if(!chain->GetBlocks(writer))
      return session->Send(NewInternalServerErrorResponse(session, "Cannot list block chain blocks."));
    return session->Send(NewOkResponse(session, body));
  }

  void ChainController::HandleGetBlockChainHead(HttpSession* session, const HttpRequestPtr& request){
    BlockChain* chain = BlockChain::GetInstance();
    BlockPtr head = chain->GetHead();
    HttpResponsePtr response = NewOkResponse(session, head);
    return session->Send(response);
  }

  void ChainController::HandleGetBlockChainBlock(HttpSession* session, const HttpRequestPtr& request){
    BlockChain* chain = BlockChain::GetInstance();

    Hash hash = request->GetHashParameterValue();
    if(!chain->HasBlock(hash))
      return session->Send(NewNoContentResponse(session, hash));

    BlockPtr blk = chain->GetBlock(hash);
    return session->Send(NewOkResponse(session, blk));
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE