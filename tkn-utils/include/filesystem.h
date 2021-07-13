#ifndef TOKEN_UTILS_FILESYSTEM_H
#define TOKEN_UTILS_FILESYSTEM_H

#include <cstdio>

#include "hash.h"
#include "version.h"
#include "timestamp.h"

namespace token{
namespace internal{
static inline bool
Flush(FILE* file){
  if(!file){
    DLOG(WARNING) << "file is null.";
    return false;
  }
  if(fflush(file) != 0){
    DLOG(ERROR) << "cannot flush file: " << strerror(errno);
    return false;
  }
  return true;
}

static inline bool
Close(FILE* file){
  if(!file){
    DLOG(WARNING) << "cannot close file, file is null.";
    return false; //should we return true for this case?
  }

  if(fclose(file) != 0){
    DLOG(ERROR) << "cannot close file: " << strerror(errno);
    return false;
  }
  return true;
}

static inline bool
Seek(FILE* file, const int64_t pos, int whence=SEEK_SET){
  if(!file){
    DLOG(WARNING) << "cannot seek file, file is not open.";
    return false;
  }

  if(fseek(file, pos, whence) != 0){
    DLOG(ERROR) << "cannot seek to " << pos << " in file: " << strerror(errno);
    return false;
  }
  return true;
}
}

  bool ReadBytes(FILE* file, uint8_t* data, const int64_t& nbytes);
}

#endif//TOKEN_UTILS_FILESYSTEM_H