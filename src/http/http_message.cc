#include "http/http_message.h"
#include "http/http_request.h"

namespace token{
  HttpMessagePtr HttpMessage::From(Session<HttpMessage>* session, const BufferPtr& buffer){
    HttpMessagePtr message = HttpRequest::NewInstance((HttpSession*)session, buffer);
    buffer->SetReadPosition(buffer->GetBufferSize());
    return message;
  }
}