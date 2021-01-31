#ifdef TOKEN_ENABLE_REST_SERVICE

#include "pool.h"
#include "wallet.h"
#include "deeplink.h"
#include "server/http/service.h"
#include "server/http/controller/wallet_controller.h"

namespace Token{
  static inline bool
  ParseInt(const std::string& val, int* result){
    for(auto c : val){
      if(!isdigit(c)){
        (*result) = 0;
        return false;
      }
    }

    (*result) = atoi(val.data());
    return true;
  }

  void WalletController::HandleGetUserWalletTokenCode(HttpSession* session, const HttpRequestPtr& request){
    User user = request->GetUserParameterValue();
    Hash hash = request->GetHashParameterValue();

    if(!ObjectPool::HasUnclaimedTransaction(hash))
      return session->Send(NewNoContentResponse(session, hash));

    int width = DEEPLINK_DEFAULT_WIDTH;
    int height = DEEPLINK_DEFAULT_HEIGHT;
    if(request->HasQueryParameter("scale")){
      int scale;
      if(!ParseInt(request->GetQueryParameterValue("scale"), &scale)){
        std::stringstream ss;
        ss << "scale factor of " << request->GetQueryParameterValue("scale") << " is not a valid number";
        return session->Send(NewInternalServerErrorResponse(session, ss));
      }
      width = scale;
      height = scale;
    } else if(request->HasQueryParameter("width")){
      if(!request->HasQueryParameter("height"))
        return session->Send(NewInternalServerErrorResponse(session, "Please specify a height"));
      if(!ParseInt(request->GetQueryParameterValue("width"), &width)){
        std::stringstream ss;
        ss << "width of " << request->GetQueryParameterValue("width") << " is not a valid number.";
        return session->Send(NewInternalServerErrorResponse(session, ss));
      }

      if(!ParseInt(request->GetQueryParameterValue("height"), &height)){
        std::stringstream ss;
        ss << "height of " << request->GetQueryParameterValue("height") << " is not a valid number.";
        return session->Send(NewInternalServerErrorResponse(session, ss));
      }
    }

    if(width < DEEPLINK_MIN_WIDTH || width > DEEPLINK_MAX_WIDTH){
      std::stringstream ss;
      ss << "Width of " << width << " is invalid. Width constraints: [" << DEEPLINK_MIN_WIDTH << "-" << DEEPLINK_MAX_WIDTH << "]";
      return session->Send(NewInternalServerErrorResponse(session, ss));
    }

    if(height < DEEPLINK_MIN_HEIGHT || height > DEEPLINK_MAX_HEIGHT){
      std::stringstream ss;
      ss << "Height of " << height << " is invalid. Height constraints: [" << DEEPLINK_MIN_HEIGHT << "-" << DEEPLINK_MAX_HEIGHT << "]";
      return session->Send(NewInternalServerErrorResponse(session, ss));
    }

    DeeplinkGenerator generator(width, height);

    Bitmap bitmap = generator.Generate(hash);
    BufferPtr data = Buffer::NewInstance(bitmap.width()*bitmap.height());
    if(!WritePNG(data, bitmap)){
      std::stringstream ss;
      ss << "Couldn't generate deeplink for: " << hash;
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
        writer.Key("user");
        user.Write(writer);

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