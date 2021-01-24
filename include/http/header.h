#ifndef TOKEN_INCLUDE_HTTP_HEADER_H
#define TOKEN_INCLUDE_HTTP_HEADER_H

#include <unordered_map>

namespace Token{
#define HTTP_HEADER_CONTENT_TYPE "Content-Type"
#define HTTP_HEADER_CONTENT_LENGTH "Content-Length"

#define HTTP_CONTENT_TYPE_TEXT_PLAIN "text/plain"
#define HTTP_CONTENT_TYPE_APPLICATION_JSON "application/json; charset=utf-8"
  #define HTTP_CONTENT_TYPE_IMAGE_PNG "image/png"

  typedef std::unordered_map<std::string, std::string> HttpHeadersMap;
}

#endif //TOKEN_INCLUDE_HTTP_HEADER_H