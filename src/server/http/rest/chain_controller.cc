#ifdef TOKEN_ENABLE_REST_SERVICE

#include "blockchain.h"
#include "server/http/rest/chain_controller.h"

namespace Token{
  void BlockChainController::HandleGetBlockChain(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewNoContentResponse(session, "Cannot get the list of blocks in the blockchain."));
  }

  void BlockChainController::HandleGetBlockChainHead(HttpSession* session, const HttpRequestPtr& request){
    BlockPtr head = BlockChain::GetHead();
    HttpResponsePtr response = NewOkResponse(session, head);
    return session->Send(response);
  }

  void BlockChainController::HandleGetBlockChainBlock(HttpSession* session, const HttpRequestPtr& request){
    Hash hash = request->GetHashParameterValue();
    if(!BlockChain::HasBlock(hash))
      return session->Send(NewNoContentResponse(session, hash));
    BlockPtr blk = BlockChain::GetBlock(hash);
    return session->Send(NewOkResponse(session, blk));
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE