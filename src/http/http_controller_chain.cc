#ifdef TOKEN_ENABLE_REST_SERVICE

#include "blockchain.h"
#include "http/http_service.h"
#include "http/http_controller_chain.h"

namespace token{
  void ChainController::HandleGetBlockChain(HttpSession* session, const HttpRequestPtr& request){
    Json::String body;
    Json::Writer writer(body);
    if(!BlockChain::GetBlocks(writer))
      return session->Send(NewInternalServerErrorResponse(session, "Cannot list block chain blocks."));
    return session->Send(NewOkResponse(session, body));
  }

  void ChainController::HandleGetBlockChainHead(HttpSession* session, const HttpRequestPtr& request){
    BlockPtr head = BlockChain::GetHead();
    HttpResponsePtr response = NewOkResponse(session, head);
    return session->Send(response);
  }

  void ChainController::HandleGetBlockChainBlock(HttpSession* session, const HttpRequestPtr& request){
    Hash hash = request->GetHashParameterValue();
    if(!BlockChain::HasBlock(hash))
      return session->Send(NewNoContentResponse(session, hash));
    BlockPtr blk = BlockChain::GetBlock(hash);
    return session->Send(NewOkResponse(session, blk));
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE