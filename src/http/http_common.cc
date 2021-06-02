#include "http/http_common.h"
#include "json.h"

namespace token{
  namespace http{
    bool ErrorMessage::ToJson(Json::Writer& writer) const{
      if (!writer.StartObject())
        return false;
      if (!json::SetField(writer, "code", code_))
        return false;
      if (!json::SetField(writer, "message", message_))
        return false;
      return writer.EndObject();
    }
  }
}
