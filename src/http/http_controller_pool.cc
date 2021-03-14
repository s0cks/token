#ifdef TOKEN_ENABLE_REST_SERVICE

#include "pool.h"
#include "http/http_service.h"
#include "http/http_controller_pool.h"

namespace token{
#define DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER(Method, Path, Name) \
  HTTP_CONTROLLER_ROUTE_HANDLER(PoolController, Name)

  FOR_EACH_POOL_CONTROLLER_ENDPOINT(DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER)
#undef DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER

  HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetBlock){
    Hash hash = request->GetHashParameterValue();
    if(!ObjectPool::HasBlock(hash))
      return session->Send(NewNoContentResponse(session, hash));
    BlockPtr blk = ObjectPool::GetBlock(hash);
    return session->Send(NewOkResponse(session, blk));
  }

  HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetBlocks){

  }

  HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetTransaction){
    Hash hash = request->GetHashParameterValue();
    if(!ObjectPool::HasTransaction(hash))
      return session->Send(NewNoContentResponse(session, hash));
    TransactionPtr tx = ObjectPool::GetTransaction(hash);
    return session->Send(NewOkResponse(session, tx));
  }

  HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetTransactions){
    return session->Send(NewNotImplementedResponse(session, "Not Implemented."));
  }

  HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetUnclaimedTransaction){
    Hash hash = request->GetHashParameterValue();
    if(!ObjectPool::HasUnclaimedTransaction(hash))
      return session->Send(NewNoContentResponse(session, hash));
    UnclaimedTransactionPtr utxo = ObjectPool::GetUnclaimedTransaction(hash);
    return session->Send(NewOkResponse(session, utxo));
  }

  HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetUnclaimedTransactions){
    Json::String body;
    Json::Writer writer(body);
    writer.StartObject();
    {
      writer.Key("data");
      if(!ObjectPool::GetUnclaimedTransactions(writer))
        return session->Send(NewInternalServerErrorResponse(session, "Cannot get the list of unclaimed transactions in the object pool."));
    }
    writer.EndObject();
    return session->Send(NewOkResponse(session, body));
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE