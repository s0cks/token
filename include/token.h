#ifndef TOKEN_TOKEN_H
#define TOKEN_TOKEN_H

#include <iostream>
#include <string>

#define TOKEN_MAJOR_VERSION 1
#define TOKEN_MINOR_VERSION 0
#define TOKEN_REVISION_VERSION 0

namespace Token{
    std::string GetVersion();
}

#endif //TOKEN_TOKEN_H