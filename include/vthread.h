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

namespace token{
  ThreadId GetCurrentThread();
  std::string GetThreadName(const ThreadId& thread);
  bool SetThreadName(const ThreadId& thread, const char* name);
  bool ThreadStart(ThreadId* thread, const char* name, ThreadHandlerFunction func, uword parameter);
  bool ThreadJoin(const ThreadId& thread);

#define LOG_THREAD(LevelName) \
  LOG(LevelName) << "[" << GetThreadName(GetCurrentThread()) << "] "

#define LOG_THREAD_IF(LevelName, Condition) \
  LOG(LevelName) << "[" << GetThreadName(GetCurrentThread()) << "] "

#define DLOG_THREAD(LevelName) \
  DLOG(LevelName) << "[" << GetThreadName(GetCurrentThread()) << "] "

#define DLOG_THREAD_IF(LevelName, Condition) \
  DLOG_IF(LevelName, Condition) << "[" << GetThreadName(GetCurrentThread()) << "] "
}

#endif //TOKEN_VTHREAD_H