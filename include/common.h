#ifndef TOKEN_COMMON_H
#define TOKEN_COMMON_H

#include <time.h>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <chrono>
#include <vector>
#include <random>
#include <string>
#include <sstream>
#include <uv.h>
#include <signal.h>
#include <sys/stat.h>
#include <uuid/uuid.h>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/rsa.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>
#include <cryptopp/pssr.h>
#include <iomanip>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wformat-truncation"

//TODO: cleanup
namespace Token{
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

  typedef int64_t Timestamp;

  static Timestamp
  GetCurrentTimestamp(){
    return time(NULL);
  }

  static inline std::string
  GetTimestampFormattedReadable(Timestamp timestamp = GetCurrentTimestamp()){
    //TODO: fix usage of gmtime
    struct tm* timeinfo = gmtime((time_t*) &timestamp);
    char buff[256];
    strftime(buff, sizeof(buff), "%m/%d/%Y %H:%M:%S", timeinfo);
    return std::string(buff);
  }

  static inline std::string
  GetTimestampFormattedFileSafe(Timestamp timestamp = GetCurrentTimestamp()){
    //TODO: fix usage of gmtime
    struct tm* timeinfo = gmtime((time_t*) &timestamp);
    char buff[256];
    strftime(buff, sizeof(buff), "%Y%m%d-%H%M%S", timeinfo);
    return std::string(buff);
  }

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
    std::ifstream f(name.c_str());
    return f.good();
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

  static inline bool
  HasEnvironmentVariable(const std::string& name){ //TODO: move HasEnvironmentVariable to common
    char* val = getenv(name.data());
    return val != NULL;
  }

  static inline std::string
  GetEnvironmentVariable(const std::string& name){ //TODO: move GetEnvironmentVariable to common
    char* val = getenv(name.data());
    if(val == NULL) return "";
    return std::string(val);
  }
}

// --path "/usr/share/ledger"
DECLARE_string(path);
// --enable-snapshots
DECLARE_bool(enable_snapshots);
// --num-worker-threads
DECLARE_int32(num_workers);

#ifdef TOKEN_DEBUG
DECLARE_int64(miner_interval);
DECLARE_bool(append_test);
#endif//TOKEN_DEBUG

#ifdef TOKEN_ENABLE_SERVER
// --remote "localhost:8080"
DECLARE_string(remote);
// --server-port 8080
DECLARE_int32(server_port);
// --num-peer-threads
DECLARE_int32(num_peers);
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
// --healthcheck-port 8081
DECLARE_int32(healthcheck_port);
#endif//TOKEN_ENABLE_HEALTH_SERVICE

#ifdef TOKEN_ENABLE_REST_SERVICE
// --service-port 8082
DECLARE_int32(service_port);
#endif//TOKEN_ENABLE_REST_SERVICE

static inline bool
IsValidPort(int32_t port){
  return port > 0;
}

#define TOKEN_BLOCKCHAIN_HOME (FLAGS_path)

#endif //TOKEN_COMMON_H