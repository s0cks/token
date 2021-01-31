#ifndef TOKEN_HTTP_MESSAGE_H
#define TOKEN_HTTP_MESSAGE_H

#include <memory>
#include "message.h"
#include "utils/buffer.h"

namespace Token{
  class HttpMessage;
  typedef std::shared_ptr<HttpMessage> HttpMessagePtr;

  class HttpSession;
  class HttpMessage : public Message{
   protected:
    HttpMessage() = default;
   public:
    virtual ~HttpMessage() = default;

    virtual const char* GetName() const{
      return "HttpMessage";
    }

    static HttpMessagePtr From(HttpSession* session, const BufferPtr& buffer);
  };
}

#endif//TOKEN_HTTP_MESSAGE_H