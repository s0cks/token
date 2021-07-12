#ifndef TKN_OS_THREAD_H
#define TKN_OS_THREAD_H

#include <string>

#include "platform.h"
#ifdef OS_IS_LINUX
  #include "os_thread_linux.h"
#elif OS_IS_WINDOWS
  #include "os_thread_windows.h"
#elif OS_IS_OSX
  #include "os_thread_osx.h"
#endif

namespace token{
  namespace platform {
    ThreadId GetCurrentThreadId();
    std::string GetThreadName(const ThreadId &thread);
    bool SetThreadName(const ThreadId &thread, const char *name);
    bool ThreadStart(ThreadId *thread, const char *name, const ThreadHandler &func, uword parameter);
    bool ThreadJoin(const ThreadId &thread);

    int ThreadEquals(const ThreadId& lhs, const ThreadId& rhs);

    static inline std::string
    GetCurrentThreadName(){
      return GetThreadName(GetCurrentThreadId());
    }

    static inline bool
    SetCurrentThreadName(const char* name){
      return SetThreadName(GetCurrentThreadId(), name);
    }
  }
}

#endif//TKN_OS_THREAD_H