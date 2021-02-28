#ifndef TOKEN_HTTP_CLIENT_H
#define TOKEN_HTTP_CLIENT_H

#include <uv.h>
#include "http/http_session.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace token{
  class HttpClient{
   private:
    NodeAddress address_;
   public:
    HttpClient(const NodeAddress& address):
      address_(address){}
    ~HttpClient() = default;

    HttpResponsePtr Send(const HttpRequestPtr& request);
  };
}

#endif//TOKEN_HTTP_CLIENT_H