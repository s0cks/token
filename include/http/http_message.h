#ifndef TOKEN_HTTP_MESSAGE_H
#define TOKEN_HTTP_MESSAGE_H

#include <memory>
#include "message.h"
#include "utils/buffer.h"

namespace Token{
  template<class M>
  class Session;

  class HttpMessage;
  typedef std::shared_ptr<HttpMessage> HttpMessagePtr;

  class HttpMessage : public Message{
   protected:
    HttpMessage() = default;
   public:
    virtual ~HttpMessage() = default;

    virtual const char* GetName() const{
      return "HttpMessage";
    }

    static HttpMessagePtr From(Session<HttpMessage>* session, const BufferPtr& buffer);
  };
}

#endif//TOKEN_HTTP_MESSAGE_H