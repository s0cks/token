#include "http/http_message.h"
#include "http/http_request.h"

namespace token{
  namespace http{
    MessagePtr Message::From(const SessionPtr& session, const BufferPtr& buffer){
      RequestPtr request = RequestParser::ParseRequest(session, buffer);
      buffer->SetReadPosition(buffer->GetBufferSize());
      return request;
    }
  }
}