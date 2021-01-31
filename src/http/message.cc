#include "http/message.h"
#include "http/request.h"

namespace Token{
  HttpMessagePtr HttpMessage::From(HttpSession* session, const BufferPtr& buffer){
    HttpMessagePtr message = HttpRequest::NewInstance(session, buffer);
    buffer->SetReadPosition(buffer->GetBufferSize());
    return message;
  }
}