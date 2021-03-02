#ifndef TOKEN_INCLUDE_HTTP_HEADER_H
#define TOKEN_INCLUDE_HTTP_HEADER_H

#include <unordered_map>
#include "object.h"
#include "configuration.h"

namespace token{
#define HTTP_HEADER_DATE "Date"
#define HTTP_HEADER_SERVER "Server"
#define HTTP_HEADER_X_NODE_ID "X-Token-NodeId"
#define HTTP_HEADER_X_NODE_VERSION "X-Token-Version"
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

  static inline std::string
  GetServerHeaderValue(){
    std::stringstream ss;
    ss << "Node/" << Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    return ss.str();
  }

  static inline std::string
  GetXNodeIDHeaderValue(){
    std::stringstream ss;
    ss << "Node/" << ConfigurationManager::GetID(TOKEN_CONFIGURATION_NODE_ID).ToStringAbbreviated();
    return ss.str();
  }

  static inline std::string
  GetXNodeVersionHeaderValue(){
    std::stringstream ss;
    ss << Version(TOKEN_MAJOR_VERSION, TOKEN_MINOR_VERSION, TOKEN_REVISION_VERSION);
    return ss.str();
  }

  static inline void
  InitHttpResponseHeaders(HttpHeadersMap& headers){
    SetHttpHeader(headers, HTTP_HEADER_CONNECTION, "close");
    SetHttpHeader(headers, HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
    SetHttpHeader(headers, HTTP_HEADER_DATE, Clock::now());
    SetHttpHeader(headers, HTTP_HEADER_SERVER, GetServerHeaderValue());
    SetHttpHeader(headers, HTTP_HEADER_X_NODE_ID, GetXNodeIDHeaderValue());
    SetHttpHeader(headers, HTTP_HEADER_X_NODE_VERSION, GetXNodeVersionHeaderValue());
  }
}

#endif //TOKEN_INCLUDE_HTTP_HEADER_H