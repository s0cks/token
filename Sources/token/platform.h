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
 typedef std::chrono::system_clock Clock;
 typedef Clock::time_point Timestamp;
 typedef Clock::duration Duration;

 typedef int8_t Byte;
 static constexpr int kInt8SizeLog2 = 0;
 static constexpr int kInt8Size = 1 << kInt8SizeLog2;
 typedef uint8_t UnsignedByte;

 typedef int16_t Short;
 static constexpr int kInt16SizeLog2 = 1;
 static constexpr int kInt16Size = 1 << kInt16SizeLog2;
 typedef uint16_t UnsignedShort;

 typedef int32_t Int;
 static constexpr int kInt32SizeLog2 = 2;
 static constexpr int kInt32Size = 1 << kInt32SizeLog2;
 typedef uint32_t UnsignedInt;

 typedef int64_t Long;
 static constexpr int kInt64SizeLog2 = 3;
 static constexpr int kInt64Size = 1 << kInt64SizeLog2;
 typedef uint64_t UnsignedLong;

 static constexpr int kBitsPerByteLog2 = 3;
 static constexpr int kBitsPerByte = 1 << kBitsPerByteLog2;
 static constexpr int kBitsPerInt8 = kInt8Size * kBitsPerByte;
 static constexpr int kBitsPerInt16 = kInt16Size * kBitsPerByte;
 static constexpr int kBitsPerInt32 = kInt32Size * kBitsPerByte;
 static constexpr int kBitsPerInt64 = kInt64Size * kBitsPerByte;

 typedef intptr_t word;
 typedef uintptr_t uword;

#ifdef ARCHITECTURE_IS_X32
 static constexpr int kWordSizeLog2 = kInt32SizeLog2;
#else
 static constexpr int kWordSizeLog2 = kInt64SizeLog2;
#endif
 static constexpr int kWordSize = 1 << kWordSizeLog2;

 static constexpr int kBitsPerWordLog2 = kWordSizeLog2 + kBitsPerByteLog2;
 static constexpr int kBitsPerWord = 1 << kBitsPerWordLog2;

 static constexpr uword kUWordOne = 1U;
}

#endif//TKN_PLATFORM_H