#ifndef TKN_UTILS_STRING_UTILS_H
#define TKN_UTILS_STRING_UTILS_H

#include <string>
#include <vector>

namespace token{
  namespace utils{
    static inline void
    SplitString(const std::string& str, std::vector<std::string>& cont, char delimiter = ' '){
      if(str.find(delimiter) == std::string::npos){ // delimiter not found, return whole
        cont.push_back(str);
        return;
      }
      std::stringstream ss(str);
      std::string token;
      while(std::getline(ss, token, delimiter)) cont.push_back(token);
    }
  }
}

#endif//TKN_UTILS_STRING_UTILS_H