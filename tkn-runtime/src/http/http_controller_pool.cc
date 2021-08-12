#include "pool.h"
#include "http/http_session.h"
#include "http/http_controller_pool.h"

namespace token{
  namespace http{
#define DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER(Method, Path, Name) \
  HTTP_CONTROLLER_ROUTE_HANDLER(PoolController, Name)

    FOR_EACH_POOL_CONTROLLER_ENDPOINT(DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER)
#undef DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER

    HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetBlock){
      Hash hash = request->GetHashParameterValue();
      DLOG(INFO) << "searching for " << hash << " (Block)....";
      if(!GetPool().HasBlock(hash))
        return session->Send(NewNoContentResponse(hash));
      BlockPtr blk = GetPool().GetBlock(hash);
      return session->Send(NewOkResponse(blk));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetAllBlocks){
      DLOG(INFO) << "handling /pool/blocks...";

      json::String body;
      json::Writer writer(body);
      if(!GetPool().GetBlocks(writer))
        return session->Send(NewInternalServerErrorResponse("cannot get list of blocks from pool."));
      return session->Send(NewOkResponse(body));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetPoolInfo){
      PoolStats stats;
      GetPool().GetPoolStats(stats);

      json::String body;
      json::Writer writer(body);
      if(!writer.StartObject())
        return session->Send(NewInternalServerErrorResponse("An error has occurred"));

      {
        if(!json::SetField(writer, "data", stats))
          return session->Send(NewInternalServerErrorResponse("cannot write stats to json"));
      }

      if(!writer.EndObject())
        return session->Send(NewInternalServerErrorResponse("An error has occurred"));

      return session->Send(NewOkResponse(body));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetTransaction){
      Hash hash = request->GetHashParameterValue();
      if(!GetPool().HasUnsignedTransaction(hash))
        return session->Send(NewNoContentResponse(hash));
      UnsignedTransactionPtr tx = GetPool().GetUnsignedTransaction(hash);
      return session->Send(NewOkResponse(tx));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetTransactions){
      return session->Send(NewNotImplementedResponse("Not Implemented."));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetUnclaimedTransaction){
      Hash hash = request->GetHashParameterValue();
      if(!GetPool().HasUnclaimedTransaction(hash))
        return session->Send(NewNoContentResponse(hash));
      UnclaimedTransactionPtr utxo = GetPool().GetUnclaimedTransaction(hash);
      return session->Send(NewOkResponse(utxo));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(PoolController, GetUnclaimedTransactions){
      json::String body;
      json::Writer writer(body);
      writer.StartObject();
      {
        writer.Key("data");
        if(!GetPool().GetUnclaimedTransactions(writer))
          return session->Send(NewInternalServerErrorResponse("Cannot get the list of unclaimed transactions in the object pool."));
      }
      writer.EndObject();
      return session->Send(NewOkResponse(body));
    }
  }
}