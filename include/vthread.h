#ifndef TOKEN_VTHREAD_H
#define TOKEN_VTHREAD_H

#include "common.h"

#ifdef OS_IS_LINUX
#include "vthread_linux.h"
#elif OS_IS_WINDOWS
#error "Unsupported"
#elif OS_IS_OSX
#error "Unsupported"
#endif

namespace Token{
  class Thread{
   protected:
    static bool Start(ThreadId *thread, const char *name, ThreadHandlerFunction function, uword parameter);
    static bool Stop(ThreadId thread);
   public:
    virtual ~Thread() = delete;
  };
}

#endif //TOKEN_VTHREAD_H