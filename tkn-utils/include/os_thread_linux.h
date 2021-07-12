#ifndef TKN_OS_THREAD_LINUX_H
#define TKN_OS_THREAD_LINUX_H

#ifndef TKN_OS_THREAD_H
#error "Please #include <os_thread.h> directly instead."
#endif//TKN_OS_THREAD_H

#include <pthread.h>
#include "platform.h"

namespace token{
  static const int kThreadNameMaxLength = 16;
  static const int kThreadMaxResultLength = 128;

  typedef pthread_key_t ThreadLocalKey;
  typedef pthread_t ThreadId;
  typedef void (*ThreadHandler)(uword parameter);
}

#endif//TKN_OS_THREAD_LINUX_H