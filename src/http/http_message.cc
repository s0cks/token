#include "http/http_message.h"
#include "http/http_request.h"

namespace token{
  HttpMessagePtr HttpMessage::From(SessionBase<HttpMessage>* session, const BufferPtr& buffer){
    HttpRequestPtr message = HttpRequestParser::ParseRequest((HttpSession*)session, buffer);
    buffer->SetReadPosition(buffer->GetBufferSize());
    return message;
  }
}