#include <cerrno>
#include <glog/logging.h>

#include "filesystem.h"

namespace token{
  bool ReadBytes(FILE* file, uint8_t* data, const int64_t& size){
    if(fread(data, sizeof(uint8_t), size, file) != (sizeof(uint8_t)*size)){
      LOG(WARNING) << "cannot read " << size << " bytes from file: " << strerror(errno);
      return false;
    }
    return true;
  }
}