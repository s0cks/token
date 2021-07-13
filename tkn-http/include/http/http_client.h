#ifndef TOKEN_HTTP_CLIENT_H
#define TOKEN_HTTP_CLIENT_H

#include <uv.h>
#include "http_session.h"
#include "http_request.h"
#include "http_response.h"

namespace token{
  class HttpClient{
   private:
    NodeAddress address_;
   public:
    explicit HttpClient(const NodeAddress& address):
      address_(address){}
    ~HttpClient() = default;

    http::ResponsePtr Send(const http::RequestPtr& request);
  };
}

#endif//TOKEN_HTTP_CLIENT_H