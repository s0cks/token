#ifndef TKN_PLATFORM_H
#define TKN_PLATFORM_H

#include <chrono>
#include <cstdint>

#if defined(__linux__) || defined(__FreeBSD__)
  #define OS_IS_LINUX 1
#elif defined(__APPLE__)
  #define OS_IS_OSX 1
#elif defined(_WIN32)
  #define OS_IS_WINDOWS 1
#endif

#if defined(_M_X64) || defined(__x86_64__)
  #define ARCHITECTURE_IS_X64 1
#elif defined(_M_IX86) || defined(__i386__)
  #define ARCHITECTURE_IS_X32 1
#elif defined(__ARMEL__)
  #define ARCHITECTURE_IS_ARM 1
#elif defined(__aarch64__)
  #define ARCHITECTURE_IS_ARM64 1
#else
  #error "Cannot determine CPU architecture"
#endif

namespace token{
  namespace platform{
    typedef std::chrono::system_clock Clock;
    typedef Clock::time_point Timestamp;
    typedef Clock::duration Duration;
  }

  typedef intptr_t word;
  typedef uintptr_t uword;
}

#endif//TKN_PLATFORM_H