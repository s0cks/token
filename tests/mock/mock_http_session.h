#ifndef TOKEN_MOCKS_HTTP_SESSION_H
#define TOKEN_MOCKS_HTTP_SESSION_H

#include <gmock/gmock.h>
#include "http/http_status.h"
#include "http/http_session.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace token{
  class MockHttpSession : public HttpSession{
   public:
    MockHttpSession():
      HttpSession(){}
    ~MockHttpSession() = default;

    MOCK_METHOD(void, Send, (const HttpMessagePtr&), ());
  };
}

#endif//TOKEN_MOCKS_HTTP_SESSION_H