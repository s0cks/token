#ifndef TOKEN_INCLUDE_HTTP_HEADER_H
#define TOKEN_INCLUDE_HTTP_HEADER_H

#include <unordered_map>
#include "object.h"

namespace Token{
#define HTTP_HEADER_DATE "Date"
#define HTTP_HEADER_SERVER "Server"
#define HTTP_HEADER_CONNECTION "Connection"
#define HTTP_HEADER_CONTENT_TYPE "Content-Type"
#define HTTP_HEADER_CONTENT_LENGTH "Content-Length"
#define HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN "Access-Control-Allow-Origin"

#define HTTP_CONTENT_TYPE_TEXT_PLAIN "text/plain; charset=utf-8"
#define HTTP_CONTENT_TYPE_APPLICATION_JSON "application/json; charset=utf-8"
#define HTTP_CONTENT_TYPE_IMAGE_PNG "image/png"

  typedef std::unordered_map<std::string, std::string> HttpHeadersMap;

  static inline bool
  SetHttpHeader(HttpHeadersMap& headers, const std::string& name, const std::string& value){
    return headers.insert({ name, value }).second;
  }

  static inline bool
  SetHttpHeader(HttpHeadersMap& headers, const std::string& name, const std::stringstream& val){
    return headers.insert({ name, val.str() }).second;
  }

  static inline bool
  SetHttpHeader(HttpHeadersMap& headers, const std::string& name, const long& val){
    std::stringstream ss;
    ss << val;
    return SetHttpHeader(headers, name, ss);
  }

  static inline bool
  SetHttpHeader(HttpHeadersMap& headers, const std::string& name, const Timestamp& time){
    return SetHttpHeader(headers, name, FormatTimestampReadable(time));
  }

  static inline bool
  SetHttpHeader(HttpHeadersMap& headers, const std::string& name, const UUID& val){
    return SetHttpHeader(headers, name, val.str());
  }

  static inline void
  InitHttpResponseHeaders(HttpHeadersMap& headers){
    SetHttpHeader(headers, HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
    SetHttpHeader(headers, "Date", Clock::now());
    //SetHttpHeader(headers, "Server", Server::GetID());
  }
}

#endif //TOKEN_INCLUDE_HTTP_HEADER_H