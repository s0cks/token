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
#include <signal.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <fstream>
#include <iomanip>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include <cmath>

#include <leveldb/status.h>


#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wformat-truncation"

//TODO: cleanup
namespace token{
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

#define TOKEN_MAJOR_VERSION 1
#define TOKEN_MINOR_VERSION 0
#define TOKEN_REVISION_VERSION 0

  static const size_t kWordSize = sizeof(uintptr_t);
  static const size_t kBitsPerByte = 8;
  static const size_t kBitsPerWord = sizeof(uintptr_t) * kBitsPerByte;
  static const uintptr_t kUWordOne = 1U;

  typedef uintptr_t uword;

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

#define TOKEN_MAGIC 0xFEFE

  static bool
  EndsWith(const std::string& str, const std::string& suffix){
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
  }

  static bool
  BeginsWith(const std::string& str, const std::string prefix){
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

  template<class container>
  static inline void
  SplitString(const std::string& str, container& cont, char delimiter = ' '){
    if(str.find(delimiter) == std::string::npos){ // delimiter not found, return whole
      cont.push_back(str);
      return;
    }
    std::stringstream ss(str);
    std::string token;
    while(std::getline(ss, token, delimiter)) cont.push_back(token);
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

    SignalHandlers() = delete;
   public:
    ~SignalHandlers() = delete;

    static void Initialize(){
      signal(SIGINT, &HandleInterrupt);
      signal(SIGSEGV, &HandleSegfault);
    }
  };

#define LOG_NAMED(LevelName) \
  LOG(LevelName) << "[" << GetName() << "] "

#define LOG_UUID(LevelName) \
  LOG(LevelName) << "[" << GetUUID() << "] "
}

DECLARE_string(path);
DECLARE_bool(enable_snapshots);
DECLARE_int32(num_workers);
DECLARE_int64(mining_interval);
DECLARE_string(remote);
DECLARE_int32(server_port);
DECLARE_int32(num_peers);
DECLARE_int32(healthcheck_port);
DECLARE_int32(service_port);

#ifdef TOKEN_DEBUG
  DECLARE_bool(fresh);
  DECLARE_bool(append_test);
  DECLARE_bool(verbose);
  DECLARE_bool(no_mining);
#endif//TOKEN_DEBUG

static inline bool
IsValidPort(int32_t port){
  return port > 0;
}

#define TOKEN_BLOCKCHAIN_HOME (FLAGS_path)
#define TOKEN_VERBOSE (FLAGS_verbose)

namespace token{
  namespace Json{
    typedef rapidjson::Document Document;
    typedef rapidjson::StringBuffer String;
    typedef rapidjson::Writer<String> Writer;
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
  PrettySize(int64_t size){
    //TODO: optimize using log
    static const char* units[] = { "b", "kb", "mb", "gb", "tb", "pb" };

    std::stringstream ss;
    if(size == 0)
      return "0b";

    int i = 0;
    while (size > 1024){
      size /= 1024;
      i++;
    }

    ss << i << "." << size << units[i];
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

#define CHECK_UVRESULT(Result, Log, Message)({ \
  int err;                                           \
  if((err = Result) != 0){                         \
    Log << Message << ": " << uv_strerror(err);\
    return;                                    \
  }                                            \
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