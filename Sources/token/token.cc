#include <sstream>

#include "token/token.h"

namespace token{
 std::string GetVersion(){
   std::stringstream ss;
   ss << TOKEN_VERSION_MAJOR << ".";
   ss << TOKEN_VERSION_MINOR << ".";
   ss << TOKEN_VERSION_PATCH;
   return ss.str();
 }
}