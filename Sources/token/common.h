#ifndef TOKEN_COMMON_H
#define TOKEN_COMMON_H

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <random>
#include <string>
#include <sstream>
#include <uv.h>
#include <csignal>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <fstream>
#include <iomanip>
#include <glog/logging.h>

#include <cmath>

#include <leveldb/status.h>

#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "platform.h"

namespace token{
  static inline uword
  RoundUpPowTwo(uword x){
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
#if defined(ARCHITECTURE_IS_X64)
    x = x | (x >> 32);
#endif
    return x + 1;
  }

  static inline double
  GetPercentageOf(const uint64_t& total, const uint64_t& current){
    return (static_cast<double>(current)/static_cast<double>(total))*100.0;
  }

#define TOKEN_MAGIC 0xFEFE

  static bool
  EndsWith(const std::string& str, const std::string& suffix){
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
  }

  static bool
  BeginsWith(const std::string& str, const std::string& prefix){
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
  }

  static inline size_t
  GetFilesize(const std::string& filename){
    //TODO: optimize function for std::fstream
    std::ifstream fd(filename, std::ios::binary | std::ios::ate);
    return fd.tellg();
  }

  static inline int64_t
  GetFilesize(std::fstream& fd){
    int64_t start = fd.tellg();
    fd.seekg(0, std::ios::end);
    int64_t size = fd.tellg() - start;
    fd.seekg(0, std::ios::beg);
    return size;
  }

  static inline bool
  FileExists(const std::string& name){
    return access(name.data(), F_OK) == 0;
  }

  static inline bool
  DeleteFile(const std::string& name){
    return remove(name.c_str()) == 0;
  }

  static inline bool
  CreateDirectory(const std::string& path){
    return (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) != -1;
  }

  template<typename T>
  static inline std::vector<std::vector<T>>
  Chunk(const std::vector<T>& source, int64_t size){
    std::vector<std::vector<T>> result;
    result.reserve((source.size() + size - 1) / size);

    auto start = source.begin();
    auto end = source.end();
    while(start != end){
      auto next = std::distance(start, end) >= size
                  ? start + size
                  : end;

      result.emplace_back(start, next);
      start = next;
    }

    return result;
  }

  std::string GetVersion();

  class SignalHandlers{
   private:
    static void HandleInterrupt(int signum);
    static void HandleSegfault(int signum);
   public:
    SignalHandlers() = delete;
    ~SignalHandlers() = delete;

    static void Initialize(){
      signal(SIGINT, &HandleInterrupt);
      signal(SIGSEGV, &HandleSegfault);
    }
  };
}

static inline bool
IsValidPort(int32_t port){
  return port > 0;
}

namespace token{
  namespace json{

  }

  namespace internal{
    // memory sizes
    static const int64_t kBytes = 1;
    static const int64_t kKilobytes = 1024 * kBytes;
    static const int64_t kMegabytes = 1024 * kKilobytes;
    static const int64_t kGigabytes = 1024 * kMegabytes;
    static const int64_t kTerabytes = 1024 * kGigabytes;

    // time sizes
    static const int64_t kMilliseconds = 1;
    static const int64_t kSeconds = 1000 * kMilliseconds;
    static const int64_t kMinutes = 60 * kSeconds;
    static const int64_t kHours = 60 * kMinutes;
    static const int64_t kDays = 24 * kHours;
    static const int64_t kWeeks = 7 * kDays;
    static const int64_t kYears = 52 * kWeeks;
  }

  static inline std::string
  PrettySize(uint64_t nbytes){
    //TODO: optimize using log
    static const char* units[] = { "b", "kb", "mb", "gb", "tb", "pb" };
    if(nbytes == 0)
      return "0b";

    int i = 0;
    while(nbytes > 1024){
      nbytes /= 1024;
      i++;
    }

    std::stringstream ss;
    ss << i << "." << nbytes << units[i];
    return ss.str();
  }
}

#ifndef __TKN_FUNCTION_NAME__
  #if OS_IS_WINDOWS
    #define __TKN_FUNCTION_NAME__ __FUNCTION__
  #elif OS_IS_LINUX || OS_IS_OSX
    #define __TKN_FUNCTION_NAME__ __func__
  #endif
#endif//__TKN_FUNCTION_NAME__

#define CHECK_UVRESULT2(Level, Result, Message)({ \
    int err;                                      \
    if((err = (Result)) != 0){                    \
        LOG(Level) << (Message) << ": " << uv_strerror(err); \
        return;                                   \
    }                                             \
})
#define CHECK_UVRESULT(Result, Log, Message)({ \
  int err;                                           \
  if((err = (Result)) != 0){                         \
    Log << Message << ": " << uv_strerror(err);\
    return;                                    \
  }                                            \
})

#define VERIFY_UVRESULT2(Level, Result, Message)({ \
  int err;                                      \
  if((err = (Result)) != 0){                    \
    LOG(Level) << (Message) << ": " << uv_strerror(err); \
    return false;                               \
  }                                             \
})
#define VERIFY_UVRESULT(Result, Log, Message)({ \
  int err;                                      \
  if((err = (Result)) != 0){                    \
    Log << Message << ": " << uv_strerror(err); \
    return false;                               \
  }                                             \
})

#define NOT_IMPLEMENTED(LevelName) \
  DLOG(LevelName) << __TKN_FUNCTION_NAME__ << " is not implemented yet!"

  static inline std::ostream&
  operator<<(std::ostream& stream, const leveldb::Status& status){
    return stream << status.ToString();
  }

#endif //TOKEN_COMMON_H