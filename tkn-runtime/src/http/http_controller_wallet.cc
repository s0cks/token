#include "pool.h"
#include "wallet.h"
#include "deeplink.h"
#include "../../include/http/http_service.h"
#include "../../include/http/http_controller_wallet.h"

namespace token{
  namespace http{
#define DEFINE_HTTP_ROUTE_HANDLER(Method, Path, Name) \
  HTTP_CONTROLLER_ROUTE_HANDLER(WalletController, Name);

    FOR_EACH_WALLET_CONTROLLER_ENDPOINT(DEFINE_HTTP_ROUTE_HANDLER)
#undef DEFINE_HTTP_ROUTE_HANDLER

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

    HTTP_CONTROLLER_ENDPOINT_HANDLER(WalletController, GetUserWalletTokenCode){
      User user = request->GetUserParameterValue();
      Hash hash = request->GetHashParameterValue();

      //TODO: refactor
      ObjectPoolPtr pool = ObjectPool::GetInstance();
      if(!pool->HasUnclaimedTransaction(hash))
        return session->Send(NewNoContentResponse(hash));

      int width = DEEPLINK_DEFAULT_WIDTH;
      int height = DEEPLINK_DEFAULT_HEIGHT;
      if(request->HasQueryParameter("scale")){
        int scale;
        if(!ParseInt(request->GetQueryParameterValue("scale"), &scale)){
          std::stringstream ss;
          ss << "scale factor of " << request->GetQueryParameterValue("scale") << " is not a IsValid number";
          return session->Send(NewInternalServerErrorResponse(ss));
        }
        width = scale;
        height = scale;
      } else if(request->HasQueryParameter("width")){
        if(!request->HasQueryParameter("height"))
          return session->Send(NewInternalServerErrorResponse("Please specify a height"));
        if(!ParseInt(request->GetQueryParameterValue("width"), &width)){
          std::stringstream ss;
          ss << "width of " << request->GetQueryParameterValue("width") << " is not a IsValid number.";
          return session->Send(NewInternalServerErrorResponse(ss));
        }

        if(!ParseInt(request->GetQueryParameterValue("height"), &height)){
          std::stringstream ss;
          ss << "height of " << request->GetQueryParameterValue("height") << " is not a IsValid number.";
          return session->Send(NewInternalServerErrorResponse(ss));
        }
      }

      if(width < DEEPLINK_MIN_WIDTH || width > DEEPLINK_MAX_WIDTH){
        std::stringstream ss;
        ss << "Width of " << width << " is invalid. Width constraints: [" << DEEPLINK_MIN_WIDTH << "-" << DEEPLINK_MAX_WIDTH << "]";
        return session->Send(NewInternalServerErrorResponse(ss));
      }

      if(height < DEEPLINK_MIN_HEIGHT || height > DEEPLINK_MAX_HEIGHT){
        std::stringstream ss;
        ss << "Height of " << height << " is invalid. Height constraints: [" << DEEPLINK_MIN_HEIGHT << "-" << DEEPLINK_MAX_HEIGHT << "]";
        return session->Send(NewInternalServerErrorResponse(ss));
      }

      DeeplinkGenerator generator(width, height);

      Bitmap bitmap = generator.Generate(hash);
      BufferPtr data = internal::NewBuffer(bitmap.width() * bitmap.height());
      if(!WritePNG(data, bitmap)){
        std::stringstream ss;
        ss << "Couldn't generate deeplink for: " << hash;
        return session->Send(NewInternalServerErrorResponse(ss));
      }

      http::ResponseBuilder builder(data);
      builder.SetStatusCode(StatusCode::kOk);
      builder.SetHeader(HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_IMAGE_PNG);
      builder.SetHeader(HTTP_HEADER_CONTENT_LENGTH, data->GetWritePosition());
      return session->Send(builder.Build());
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(WalletController, GetUserWallet){
      User user = request->GetUserParameterValue();
      if(!GetWalletManager()->HasWallet(user)){
        std::stringstream ss;
        ss << "cannot find wallet for: " << user;
        return session->Send(NewNoContentResponse(ss));
      }

      Wallet wallet = GetWalletManager()->GetUserWallet(user);
      return session->Send(NewOkResponse(user, wallet));
    }

    static inline bool
    ParseInputList(http::Session* session, json::Document& doc, InputList& inputs, const char* name="inputs"){
      if(!doc.HasMember(name)){
        std::stringstream ss;
        ss << "request is missing the '" << name << "' field.";
        session->Send(NewInternalServerErrorResponse(ss));
        return false;
      }

      if(!doc[name].IsArray()){
        std::stringstream ss;
        ss << "'" << name << "' field is not an array.";
        session->Send(NewInternalServerErrorResponse(ss));
        return false;
      }

      ObjectPoolPtr pool = ObjectPool::GetInstance();
      for(auto& it : doc[name].GetArray()){
        Hash in_hash = Hash::FromHexString(it.GetString());
        LOG(INFO) << "using input: " << in_hash;
        if(!pool->HasUnclaimedTransaction(in_hash)){
          std::stringstream ss;
          ss << "Cannot find token: " << in_hash;
          session->Send(NewNotFoundResponse(ss));
          return false;
        }
        UnclaimedTransactionPtr in_val = pool->GetUnclaimedTransaction(in_hash);
        inputs.push_back(Input(in_val->GetReference(), in_val->GetUser()));
      }
      return true;
    }

#define CHECK_TYPE(Object, Name, Type) \
  if(!(Object).Is##Type()){        \
    std::stringstream ss; \
    ss << '\'' << (Name) << '\'' << " is not of type: " << #Type; \
    session->Send(NewInternalServerErrorResponse(ss));   \
    return false;                      \
  }
#define HAS_FIELD(Object, Name, Type) \
  if(!(Object).HasMember((Name))){\
    std::stringstream ss; \
    ss << "missing the '" << (Name) << "' field."; \
    session->Send(NewInternalServerErrorResponse(ss)); \
    return false;                  \
  } \
  CHECK_TYPE((Object)[(Name)], Name, Type)

    static inline bool
    ParseOutputList(http::Session* session, const json::Document& doc, OutputList& outputs, const char* name="outputs"){
      HAS_FIELD(doc, name, Array);

      for(auto& it : doc[name].GetArray()){
        CHECK_TYPE(it, name, Object); //TODO: fix name
        auto obj = it.GetObject();

        HAS_FIELD(obj, "user", String);
        User user(obj["user"].GetString());

        HAS_FIELD(obj, "product", String);
        Product product(obj["product"].GetString());

        outputs.push_back(Output(user, product));
      }
      return true;
    }

    HTTP_CONTROLLER_ENDPOINT_HANDLER(WalletController, PostUserWalletSpend){
      User user = request->GetUserParameterValue();

      json::Document doc;
      request->GetBody(doc);

      InputList inputs = {};
      LOG(INFO) << "parsing inputs....";
      if(!ParseInputList(session, doc, inputs))
        return;

      OutputList outputs = {};
      LOG(INFO) << "parsing outputs....";
      if(!ParseOutputList(session, doc, outputs))
        return;

      //TODO: refactor
      ObjectPoolPtr pool = ObjectPool::GetInstance();

      LOG(INFO) << "generating new transaction....";
      UnsignedTransactionPtr tx = UnsignedTransaction::NewInstance(Clock::now(), inputs, outputs);
      Hash hash = tx->hash();
      if(!pool->PutUnsignedTransaction(hash, tx)){
        std::stringstream ss;
        ss << "Cannot insert newly created transaction into object pool: " << hash;
        return session->Send(NewInternalServerErrorResponse(ss));
      }
      return session->Send(NewOkResponse(tx));
    }
  }
}