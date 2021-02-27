#ifndef TOKEN_INCLUDE_HTTP_METHOD_H
#define TOKEN_INCLUDE_HTTP_METHOD_H

#include <ostream>
#include "common.h"

namespace token{
  enum class HttpMethod{
    kGet,
    kPut,
    kPost,
    kDelete,
  };

  static std::ostream& operator<<(std::ostream& stream, const HttpMethod& method){
    switch(method){
      case HttpMethod::kGet:
        return stream << "GET";
      case HttpMethod::kPut:
        return stream << "PUT";
      case HttpMethod::kPost:
        return stream << "POST";
      case HttpMethod::kDelete:
        return stream << "DELETE";
      default:
        return stream << "Unknown";
    }
  }
}

#endif //TOKEN_INCLUDE_HTTP_METHOD_H