#include "token.h"
#include <sstream>

namespace Token{
    std::string GetVersion(){
        std::stringstream stream;
        stream << TOKEN_MAJOR_VERSION;
        stream << "." << TOKEN_MINOR_VERSION;
        stream << "." << TOKEN_REVISION_VERSION;
        return stream.str();
    }
}