#ifndef TKN_FILESYSTEM_H
#define TKN_FILESYSTEM_H

#include "platform.h"
#ifdef OS_IS_LINUX
  #include "filesystem_linux.h"
#elif OS_IS_WINDOWS
  #include "filesystem_windows.h"
#elif OS_IS_OSX
  #include "filesystem_osx.h"
#endif

namespace token{
  namespace platform{
    namespace fs{
      void Flush(File* file);
      void Close(File* file);
      void Seek(File* file, const int64_t& pos);
    }
  }
}

#endif//TKN_FILESYSTEM_H