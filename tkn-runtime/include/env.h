#ifndef TOKEN_ENV_H
#define TOKEN_ENV_H

#include <string>
#include <cstdlib>

namespace token{
  namespace env{
    static inline bool
    HasVariable(const char* name){
      return getenv(name) != nullptr;
    }

    static inline std::string
    GetString(const char* name){
      char* val = getenv(name);
      return val ? std::string(val) : "";
    }

    static inline std::string
    GetStringOrDefault(const char* name, const std::string& default_value){
      char* val = getenv(name);
      return val ? std::string(name) : default_value;
    }
  }
}

#endif//TOKEN_ENV_H