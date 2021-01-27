#ifdef TOKEN_ENABLE_REST_SERVICE

#include "pool.h"
#include "wallet.h"
#include "utils/qrcode.h"
#include "http/rest/wallet_controller.h"

namespace Token{
  void WalletController::HandleGetUserWalletTokenCode(HttpSession* session, const HttpRequestPtr& request){
    User user = request->GetUserParameterValue();
    Hash hash = request->GetHashParameterValue();

    if(!ObjectPool::HasUnclaimedTransaction(hash))
      return session->Send(NewNoContentResponse(session, hash));

    UnclaimedTransactionPtr utxo = ObjectPool::GetUnclaimedTransaction(hash);
    BufferPtr data = Buffer::NewInstance(65536);
    if(!WriteQRCode(data, hash)){
      std::stringstream ss;
      ss << "Cannot generate qr-code for: " << hash;
      return session->Send(NewInternalServerErrorResponse(session, ss));
    }
    return session->Send(HttpBinaryResponse::NewInstance(session, HttpStatusCode::kHttpOk, HTTP_CONTENT_TYPE_IMAGE_PNG, data));
  }

  void WalletController::HandleGetUserWallet(HttpSession* session, const HttpRequestPtr& request){
    User user = request->GetUserParameterValue();

    HttpJsonResponsePtr response = std::make_shared<HttpJsonResponse>(session, HttpStatusCode::kHttpOk);

    Json::String& body = response->GetBody();
    Json::Writer writer(body);
    writer.StartObject();
    {
      writer.Key("data");
      writer.StartObject();
      {
        Json::SetField(writer, "user", user);
        writer.Key("wallet");
        if(!WalletManager::GetWallet(user, writer)){
          std::stringstream ss;
          ss << "Cannot find wallet for: " << user;
          return session->Send(NewNoContentResponse(session, ss));
        }
      }
      writer.EndObject();
    }
    writer.EndObject();

    response->SetHeader("Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    response->SetHeader("Content-Length", body.GetSize());
    return session->Send(std::static_pointer_cast<HttpResponse>(response));
  }

  void WalletController::HandlePostUserWalletSpend(HttpSession* session, const HttpRequestPtr& request){
    User user = request->GetUserParameterValue();

    Json::Document doc;
    request->GetBody(doc);

    InputList inputs = {};
    if(!doc.HasMember("token"))
      return session->Send(NewInternalServerErrorResponse(session, "request is missing 'token' field."));
    if(!doc["token"].IsString())
      return session->Send(NewInternalServerErrorResponse(session, "'token' field is not a string."));
    Hash in_hash = Hash::FromHexString(doc["token"].GetString());
    if(!ObjectPool::HasUnclaimedTransaction(in_hash)){
      std::stringstream ss;
      ss << "Cannot find token: " << in_hash;
      return session->Send(NewNotFoundResponse(session, ss));
    }
    UnclaimedTransactionPtr in_val = ObjectPool::GetUnclaimedTransaction(in_hash);
    inputs.push_back(Input(in_val->GetReference(), in_val->GetUser())); //TODO: is this the right user?

    OutputList outputs = {};
    if(!doc.HasMember("recipient"))
      return session->Send(NewInternalServerErrorResponse(session, "request is missing 'recipient' field."));
    if(!doc["recipient"].IsString())
      return session->Send(NewInternalServerErrorResponse(session, "'recipient' field is not a string."));
    User recipient(std::string(doc["recipient"].GetString()));
    outputs.push_back(Output(recipient, in_val->GetProduct()));

    TransactionPtr tx = Transaction::NewInstance(0, inputs, outputs);
    Hash hash = tx->GetHash();
    if(!ObjectPool::PutTransaction(hash, tx)){
      std::stringstream ss;
      ss << "Cannot insert newly created transaction into object pool: " << hash;
      return session->Send(NewInternalServerErrorResponse(session, ss));
    }
    return session->Send(NewOkResponse(session, tx));
  }
}

#endif//TOKEN_ENABLE_REST_SERVICE