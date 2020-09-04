#ifndef TOKEN_COMMON_H
#define TOKEN_COMMON_H

#include <time.h>
#include <cstdint>
#include <cstdlib>
#include <chrono>
#include <vector>
#include <random>
#include <string>
#include <sstream>
#include <uv.h>
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

//TODO: cleanup
namespace Token{
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

    static const size_t kWordSize = sizeof(uintptr_t);
    static const size_t kBitsPerByte = 8;
    static const size_t kBitsPerWord = sizeof(uintptr_t)*kBitsPerByte;
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

    static uint64_t
    GetCurrentTimestamp(){
        return time(NULL);
    }

    static inline std::string
    GetTimestampFormattedReadable(uint64_t timestamp){
        //TODO: fix usage of gmtime
        struct tm* timeinfo = gmtime((time_t*)&timestamp);
        char buff[256];
        strftime(buff, sizeof(buff), "%m/%d/%Y %H:%M:%S", timeinfo);
        return std::string(buff);
    }

    static inline std::string
    GetTimestampFormattedFileSafe(uint64_t timestamp){
        //TODO: fix usage of gmtime
        struct tm* timeinfo = gmtime((time_t*)&timestamp);
        char buff[256];
        strftime(buff, sizeof(buff), "%Y%m%d-%H%M%S", timeinfo);
        return std::string(buff);
    }

    static uint32_t
    GetCurrentTime(){ //TODO: remove
        return time(NULL);
    }

    static inline std::string
    GenerateNonce(){
        CryptoPP::SecByteBlock nonce(64);
        CryptoPP::AutoSeededRandomPool prng;
        prng.GenerateBlock(nonce, nonce.size());
        std::string digest;
        CryptoPP::ArraySource source(nonce, nonce.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest)));
        return digest;
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
        std::ifstream fd(filename, std::ios::binary|std::ios::ate);
        return fd.tellg();
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
    SplitString(const std::string& str, container& cont, char delimiter=' '){
        if(str.find(delimiter) == std::string::npos){ // delimiter not found, return whole
            cont.push_back(str);
            return;
        }
        std::stringstream ss(str);
        std::string token;
        while(std::getline(ss, token, delimiter)) cont.push_back(token);
    }
}

// Command Line Flags
DECLARE_string(path);
DECLARE_uint32(port);
DECLARE_uint32(heap_size);
DECLARE_string(peer_address); //TODO: remove FLAG_peer_address
DECLARE_uint32(peer_port); //TODO: remove FLAG_peer_port

// Initialization Flags
DECLARE_string(snapshot); //TODO: rename snapshot flag

// Debug Flags
DECLARE_bool(enable_snapshots);

#define TOKEN_BLOCKCHAIN_HOME (FLAGS_path)

#endif //TOKEN_COMMON_H