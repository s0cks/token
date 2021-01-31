#ifdef TOKEN_ENABLE_REST_SERVICE

#include "blockchain.h"
#include "http/http_service.h"
#include "http/http_controller_chain.h"

namespace Token{
  void ChainController::HandleGetBlockChain(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewNoContentResponse(session, "Cannot get the list of blocks in the blockchain."));
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