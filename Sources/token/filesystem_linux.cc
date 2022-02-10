#include "filesystem.h"
#ifdef OS_IS_LINUX

#include <glog/logging.h>

namespace token{
  namespace platform{
    namespace fs{
      void Flush(File* file){
        if(!file){
          DLOG(WARNING) << "file is null.";
          return;
        }

        if(fflush(file) != 0){
          LOG(FATAL) << "cannot flush file: " << strerror(errno);
          return;
        }
      }

      void Close(File* file){
        if(!file){
          DLOG(WARNING) << "file is null.";
          return;
        }

        if(fclose(file) != 0){
          LOG(FATAL) << "cannot close file: " << strerror(errno);
          return;
        }
      }

      void Seek(File* file, const int64_t& pos){
        if(!file){
          DLOG(WARNING) << "file is null.";
          return;
        }

        if(fseek(file, pos, SEEK_SET) != 0){
          LOG(FATAL) << "cannot seek to " << pos << " in file: " << strerror(errno);
          return;
        }
      }
    }
  }
}

#endif//OS_IS_LINUX