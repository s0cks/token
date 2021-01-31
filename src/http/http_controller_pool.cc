#ifdef TOKEN_ENABLE_REST_SERVICE

#include "pool.h"
#include "http/http_service.h"
#include "http/http_controller_pool.h"

namespace Token{
  void PoolController::HandleGetStats(HttpSession* session, const HttpRequestPtr& request){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);

    Json::String& body = response->GetBody();
    Json::Writer writer(body);
    if(!ObjectPool::GetStats(writer))
      return session->Send(NewInternalServerErrorResponse(session, "Cannot get object pool stats."));

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return session->Send(std::static_pointer_cast<HttpResponse>(response));
  }

  void PoolController::HandleGetBlock(HttpSession* session, const HttpRequestPtr& request){
    Hash hash = request->GetHashParameterValue();
    if(!ObjectPool::HasBlock(hash))
      return session->Send(NewNoContentResponse(session, hash));
    BlockPtr blk = ObjectPool::GetBlock(hash);
    return session->Send(NewOkResponse(session, blk));
  }

  void PoolController::HandleGetBlocks(HttpSession* session, const HttpRequestPtr& request){

  }

  void PoolController::HandleGetTransaction(HttpSession* session, const HttpRequestPtr& request){
    Hash hash = request->GetHashParameterValue();
    if(!ObjectPool::HasTransaction(hash))
      return session->Send(NewNoContentResponse(session, hash));
    TransactionPtr tx = ObjectPool::GetTransaction(hash);
    return session->Send(NewOkResponse(session, tx));
  }

  void PoolController::HandleGetTransactions(HttpSession* session, const HttpRequestPtr& request){
    return session->Send(NewNotImplementedResponse(session, "Not Implemented."));
  }

  void PoolController::HandleGetUnclaimedTransaction(HttpSession* session, const HttpRequestPtr& request){
    Hash hash = request->GetHashParameterValue();
    if(!ObjectPool::HasUnclaimedTransaction(hash))
      return session->Send(NewNoContentResponse(session, hash));
    UnclaimedTransactionPtr utxo = ObjectPool::GetUnclaimedTransaction(hash);
    return session->Send(NewOkResponse(session, utxo));
  }

  void PoolController::HandleGetUnclaimedTransactions(HttpSession* session, const HttpRequestPtr& request){
    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);

    Json::String& body = response->GetBody();
    Json::Writer writer(body);
    writer.StartObject();
    {
      writer.Key("data");
      if(!ObjectPool::GetUnclaimedTransactions(writer))
        return session->Send(NewInternalServerErrorResponse(session, "Cannot get the list of unclaimed transactions in the object pool."));
    }
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return session->Send(std::static_pointer_cast<HttpResponse>(response));
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE