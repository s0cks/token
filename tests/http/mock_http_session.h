#ifndef TOKEN_MOCKS_HTTP_SESSION_H
#define TOKEN_MOCKS_HTTP_SESSION_H

#include <gmock/gmock.h>
#include "http/http_session.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace token{
  class MockHttpSession : public http::Session{
   public:
    MockHttpSession(): http::Session(){}
    ~MockHttpSession() = default;

    MOCK_METHOD(void, Send, (const http::MessagePtr&), ());
  };

  static inline http::SessionPtr
  NewMockHttpSession(){
    return std::make_shared<MockHttpSession>();
  }
}

#endif//TOKEN_MOCKS_HTTP_SESSION_H