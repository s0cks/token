#ifdef TOKEN_ENABLE_REST_SERVICE

#include "pool.h"
#include "wallet.h"
#include "deeplink.h"
#include "http/http_service.h"
#include "http/http_controller_wallet.h"

namespace token{
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

    if(!ObjectPool::HasUnclaimedTransaction(hash))
      return session->Send(NewNoContentResponse(session, hash));

    int width = DEEPLINK_DEFAULT_WIDTH;
    int height = DEEPLINK_DEFAULT_HEIGHT;
    if(request->HasQueryParameter("scale")){
      int scale;
      if(!ParseInt(request->GetQueryParameterValue("scale"), &scale)){
        std::stringstream ss;
        ss << "scale factor of " << request->GetQueryParameterValue("scale") << " is not a IsValid number";
        return session->Send(NewInternalServerErrorResponse(session, ss));
      }
      width = scale;
      height = scale;
    } else if(request->HasQueryParameter("width")){
      if(!request->HasQueryParameter("height"))
        return session->Send(NewInternalServerErrorResponse(session, "Please specify a height"));
      if(!ParseInt(request->GetQueryParameterValue("width"), &width)){
        std::stringstream ss;
        ss << "width of " << request->GetQueryParameterValue("width") << " is not a IsValid number.";
        return session->Send(NewInternalServerErrorResponse(session, ss));
      }

      if(!ParseInt(request->GetQueryParameterValue("height"), &height)){
        std::stringstream ss;
        ss << "height of " << request->GetQueryParameterValue("height") << " is not a IsValid number.";
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

    HttpResponseBuilder builder(session, data);
    builder.SetStatusCode(HttpStatusCode::kHttpOk);
    builder.SetHeader(HTTP_HEADER_CONTENT_TYPE, HTTP_CONTENT_TYPE_IMAGE_PNG);
    builder.SetHeader(HTTP_HEADER_CONTENT_LENGTH, data->GetWrittenBytes());
    return session->Send(builder.Build());
  }

  HTTP_CONTROLLER_ENDPOINT_HANDLER(WalletController, GetUserWallet){
    User user = request->GetUserParameterValue();
    if(!GetWalletManager()->HasWallet(user)){
      std::stringstream ss;
      ss << "cannot find wallet for: " << user;
      return session->Send(NewNoContentResponse(session, ss));
    }

    Wallet wallet = GetWalletManager()->GetUserWallet(user);

    Json::String body;
    Json::Writer writer(body);
    writer.StartObject();
    {
      writer.Key("data");
      writer.StartObject();
      {
        Json::SetField(writer, "user", user);
        Json::SetField(writer, "wallet", wallet);
      }
      writer.EndObject();
    }
    writer.EndObject();
    return session->Send(NewOkResponse(session, body));
  }

  static inline bool
  ParseInputList(HttpSession* session, const Json::Document& doc, InputList& inputs, const char* name="inputs"){
    if(!doc.HasMember(name)){
      std::stringstream ss;
      ss << "request is missing the '" << name << "' field.";
      session->Send(NewInternalServerErrorResponse(session, ss));
      return false;
    }

    if(!doc[name].IsArray()){
      std::stringstream ss;
      ss << "'" << name << "' field is not an array.";
      session->Send(NewInternalServerErrorResponse(session, ss));
      return false;
    }

    for(auto& it : doc[name].GetArray()){
      Hash in_hash = Hash::FromHexString(it.GetString());
      LOG(INFO) << "using input: " << in_hash;
      if(!ObjectPool::HasUnclaimedTransaction(in_hash)){
        std::stringstream ss;
        ss << "Cannot find token: " << in_hash;
        session->Send(NewNotFoundResponse(session, ss));
        return false;
      }
      UnclaimedTransactionPtr in_val = ObjectPool::GetUnclaimedTransaction(in_hash);
      inputs.push_back(Input(in_val->GetReference(), in_val->GetUser()));
    }
    return true;
  }

#define CHECK_TYPE(Object, Name, Type) \
  if(!(Object).Is##Type()){        \
    std::stringstream ss; \
    ss << '\'' << (Name) << '\'' << " is not of type: " << #Type; \
    session->Send(NewInternalServerErrorResponse(session, ss));   \
    return false;                      \
  }
#define HAS_FIELD(Object, Name, Type) \
  if(!(Object).HasMember((Name))){\
    std::stringstream ss; \
    ss << "missing the '" << (Name) << "' field."; \
    session->Send(NewInternalServerErrorResponse(session, ss)); \
    return false;                  \
  } \
  CHECK_TYPE((Object)[(Name)], Name, Type)

  static inline bool
  ParseOutputList(HttpSession* session, const Json::Document& doc, OutputList& outputs, const char* name="outputs"){
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

    Json::Document doc;
    request->GetBody(doc);

    InputList inputs = {};
    LOG(INFO) << "parsing inputs....";
    if(!ParseInputList(session, doc, inputs))
      return;

    OutputList outputs = {};
    LOG(INFO) << "parsing outputs....";
    if(!ParseOutputList(session, doc, outputs))
      return;

    LOG(INFO) << "generating new transaction....";
    TransactionPtr tx = Transaction::NewInstance(inputs, outputs);
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