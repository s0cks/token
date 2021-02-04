#ifndef TOKEN_VTHREAD_LINUX_H
#define TOKEN_VTHREAD_LINUX_H

#ifndef TOKEN_VTHREAD_H
#error "Please include vthread.h instead"
#endif//TOKEN_VTHREAD_H

#include <pthread.h>

namespace token{
  typedef pthread_key_t ThreadLocalKey;
  typedef pthread_t ThreadId;
  typedef void (* ThreadHandlerFunction)(uword parameter);
}

#endif //TOKEN_VTHREAD_LINUX_H