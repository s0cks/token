#include "blockchain.h"
#include "http/http_service.h"
#include "http/http_controller_chain.h"

namespace token{
  namespace http{
#define DEFINE_ENDPOINT_HANDLER(Method, Path, Name) \
  HTTP_CONTROLLER_ROUTE_HANDLER(ChainController, Name);

    FOR_EACH_CHAIN_CONTROLLER_ENDPOINT(DEFINE_ENDPOINT_HANDLER)
#undef DEFINE_ENDPOINT_HANDLER

    HTTP_CONTROLLER_ENDPOINT_HANDLER(ChainController, GetBlockChain){
      BlockChainPtr chain = GetChain();

      json::String body;
      json::Writer writer(body);

      LOG_IF(ERROR, !writer.StartObject()) << "cannot start json object.";
      LOG_IF(ERROR, !writer.Key("data", 4)) << "cannot write 'data' field name in json object.";
      if(!chain->GetBlocks(writer))
        return session->Send(NewInternalServerErrorResponse("Cannot list block chain blocks."));
      LOG_IF(ERROR, !writer.EndObject()) << "cannot end json object.";
      return session->Send(NewOkResponse(body));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(ChainController, GetBlockChainHead){
      BlockChainPtr chain = GetChain();
      BlockPtr head = chain->GetHead();
      ResponsePtr response = NewOkResponse(head);
      return session->Send(response);
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(ChainController, GetBlockChainBlock){
      BlockChainPtr chain = GetChain();

      Hash hash = request->GetHashParameterValue();
      if(!chain->HasBlock(hash))
        return session->Send(NewNoContentResponse(hash));

      BlockPtr blk = chain->GetBlock(hash);
      return session->Send(NewOkResponse(blk));
    }
  }
}