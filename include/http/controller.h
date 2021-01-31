#ifndef TOKEN_CONTROLLER_H
#define TOKEN_CONTROLLER_H

#include "router.h"
#include "request.h"
#include "response.h"

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