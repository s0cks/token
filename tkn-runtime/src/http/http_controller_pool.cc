#include "http/http_session.h"
#include "http/http_controller_pool.h"

namespace token{
  namespace http{
#define DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER(Method, Path, Name) \
    HTTP_CONTROLLER_ROUTE_HANDLER(BlockPoolController, Name)
    FOR_EACH_BLOCK_POOL_CONTROLLER_ENDPOINT(DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER)
#undef DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER

    HTTP_CONTROLLER_ENDPOINT_HANDLER(BlockPoolController, Get){
      auto hash = request->GetHashParameterValue();
      if(!pool().Has(hash))
        return session->Send(NewNoContentResponse(hash));
      auto val = pool().Get(hash);
      return session->Send(NewOkResponse(val));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(BlockPoolController, GetAll){
      HashList hashes;
      pool().GetAll(hashes);
      DLOG(INFO) << "found " << hashes.size() << " hashes...";

      json::String body;
      json::Writer writer(body);
      if(!writer.StartObject())
        return session->Send(NewInternalServerErrorResponse("error"));
      if(!json::SetField(writer, "data", hashes))
        return session->Send(NewInternalServerErrorResponse("cannot serialize hash list"));
      if(!writer.EndObject())
        return session->Send(NewInternalServerErrorResponse("error"));

      session->Send(NewOkResponse(body));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(BlockPoolController, GetStats){
      NOT_IMPLEMENTED(ERROR);
      return session->Send(NewNoContentResponse("unavailable"));
    }

#define DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER(Method, Path, Name) \
    HTTP_CONTROLLER_ROUTE_HANDLER(UnsignedTransactionPoolController, Name)
    FOR_EACH_UNSIGNED_TRANSACTION_POOL_CONTROLLER_ENDPOINT(DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER)
#undef DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER

    HTTP_CONTROLLER_ENDPOINT_HANDLER(UnsignedTransactionPoolController, Get){
      auto hash = request->GetHashParameterValue();
      if(!pool().Has(hash))
        return session->Send(NewNoContentResponse(hash));
      auto val = pool().Get(hash);
      return session->Send(NewOkResponse(val));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(UnsignedTransactionPoolController, GetAll){
      HashList hashes;
      pool().GetAll(hashes);

      json::String body;
      json::Writer writer(body);
      //TODO: serialize better
      if(!writer.StartObject())
        return session->Send(NewInternalServerErrorResponse("Error"));
      if(!json::SetField(writer, "data", hashes))
        return session->Send(NewInternalServerErrorResponse("Error"));
      if(!writer.EndObject())
        return session->Send(NewInternalServerErrorResponse("Error"));
      return session->Send(NewOkResponse(body));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(UnsignedTransactionPoolController, GetStats){
      return session->Send(NewNoContentResponse("unavailable"));
    }

#define DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER(Method, Path, Name) \
    HTTP_CONTROLLER_ROUTE_HANDLER(UnclaimedTransactionPoolController, Name)
    FOR_EACH_UNCLAIMED_TRANSACTION_POOL_CONTROLLER_ENDPOINT(DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER)
#undef DEFINE_HTTP_ENDPOINT_ROUTE_HANDLER

    HTTP_CONTROLLER_ENDPOINT_HANDLER(UnclaimedTransactionPoolController, Get){
      auto hash = request->GetHashParameterValue();
      if(!pool().Has(hash))
        return session->Send(NewNoContentResponse(hash));
      auto val = pool().Get(hash);
      return session->Send(NewOkResponse(val));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(UnclaimedTransactionPoolController, GetAll){
      HashList hashes;
      pool().GetAll(hashes);

      json::String body;
      json::Writer writer(body);
      //TODO: serialize better
      if(!writer.StartObject())
        return session->Send(NewInternalServerErrorResponse("Error"));
      if(!json::SetField(writer, "data", hashes))
        return session->Send(NewInternalServerErrorResponse("Error"));
      if(!writer.EndObject())
        return session->Send(NewInternalServerErrorResponse("Error"));
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(UnclaimedTransactionPoolController, GetStats){
      return session->Send(NewNoContentResponse("unavailable"));
    }
  }
}