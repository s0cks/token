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