#include "http/http_message.h"
#include "http/http_request.h"

namespace token{
  namespace http{
    MessagePtr Message::From(const BufferPtr& buffer){
      RequestPtr request = RequestParser::ParseRequest(buffer);
      buffer->SetReadPosition(buffer->length());
      return request;
    }
  }
}