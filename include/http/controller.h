#ifndef TOKEN_CONTROLLER_H
#define TOKEN_CONTROLLER_H

#include <rapidjson/document.h>
#include "http/router.h"
#include "http/request.h"
#include "http/response.h"

namespace Token{
  class HttpController{
   protected:
    HttpController() = delete;

    static inline void
    SendText(HttpSession* session, const std::string& body, const HttpStatusCode& status_code){
      HttpResponsePtr resp = HttpTextResponse::NewInstance(session, status_code, body);
      session->Send(resp);
    }

    static inline void
    SendText(HttpSession* session, const std::stringstream& ss, const HttpStatusCode& status_code){
      return SendText(session, ss.str(), status_code);
    }

    static inline void
    SendOk(HttpSession* session){
      return SendText(session, "Ok", STATUS_CODE_OK);
    }

    static inline void
    SendInternalServerError(HttpSession* session, const std::string& msg = "Internal Server Error"){
      return SendText(session, msg, STATUS_CODE_INTERNAL_SERVER_ERROR);
    }

    static inline void
    SendNotSupported(HttpSession* session, const std::string& path){
      std::stringstream ss;
      ss << "Not Supported.";
      return SendText(session, ss, STATUS_CODE_NOTSUPPORTED);
    }

    static inline void
    SendNotFound(HttpSession* session, const std::string& path){
      std::stringstream ss;
      ss << "Not Found: " << path;
      return SendText(session, ss, STATUS_CODE_NOTFOUND);
    }

    static inline void
    SendNotFound(HttpSession* session, const std::stringstream& ss){
      return SendText(session, ss, STATUS_CODE_NOTFOUND);
    }

    static inline void
    SendNotFound(HttpSession* session, const Hash& hash){
      std::stringstream ss;
      ss << "Not Found: " << hash;
      return SendText(session, ss, STATUS_CODE_NOTFOUND);
    }

    static inline void
    SendFile(HttpSession* session,
      const std::string& filename,
      const HttpStatusCode& status_code = STATUS_CODE_OK,
      const std::string& content_type = HTTP_CONTENT_TYPE_TEXT_PLAIN){
      HttpResponsePtr resp = HttpBinaryResponse::NewInstance(session, status_code, filename, content_type);
      session->Send(resp);
    }

    static inline void
    SendJson(HttpSession* session, const BlockPtr& val){
      JsonString body;
      ToJson(val, body);
      HttpResponsePtr resp = HttpJsonResponse::NewInstance(session, STATUS_CODE_OK, body);
      return session->Send(resp);
    }

    static inline void
    SendJson(HttpSession* session, const TransactionPtr& val){
      JsonString body;
      ToJson(val, body);
      HttpResponsePtr resp = HttpJsonResponse::NewInstance(session, STATUS_CODE_OK, body);
      return session->Send(resp);
    }

    static inline void
    SendJson(HttpSession* session, const UnclaimedTransactionPtr& val){
      JsonString body;
      ToJson(val, body);
      HttpResponsePtr resp = HttpJsonResponse::NewInstance(session, STATUS_CODE_OK, body);
      return session->Send(resp);
    }
   public:
    virtual ~HttpController() = delete;
  };

#define HTTP_CONTROLLER_ENDPOINT(Name) \
    static void Handle##Name(HttpSession* session, const HttpRequestPtr& request)

#define HTTP_CONTROLLER_GET(Path, Name) \
    router->Get((Path), &Handle##Name)
#define HTTP_CONTROLLER_PUT(Path, Name) \
    router->Put((Path), &Handle##Name)
#define HTTP_CONTROLLER_POST(Path, Name) \
    router->Post((Path), &Handle##Name)
#define HTTP_CONTROLLER_DELETE(Path, Name) \
    router->Delete((Path), &Handle##Name)

#define HTTP_CONTROLLER_INIT() \
    static inline void Initialize(HttpRouter* router)
}

#endif //TOKEN_CONTROLLER_H