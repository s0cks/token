#ifndef TOKEN_INCLUDE_HTTP_HEADER_H
#define TOKEN_INCLUDE_HTTP_HEADER_H

#include <unordered_map>

namespace Token{
  typedef std::unordered_map<std::string, std::string> HttpHeadersMap;
}

#endif //TOKEN_INCLUDE_HTTP_HEADER_H