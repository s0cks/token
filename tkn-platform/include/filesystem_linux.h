#ifndef TKN_FILESYSTEM_LINUX_H
#define TKN_FILESYSTEM_LINUX_H

#ifndef TKN_FILESYSTEM_H
#error "Please #include <filesystem.h> directly instead."
#endif//TKN_FILESYSTEM_H

#include <cstdio>

namespace token{
  namespace platform{
    namespace fs{
      typedef FILE File;
    }
  }
}

#endif//TKN_FILESYSTEM_LINUX_H