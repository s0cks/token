#ifndef TOKEN_COMMON_H
#define TOKEN_COMMON_H

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

#include "blockchain.pb.h"
#include "node.pb.h"

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

    static std::chrono::system_clock::time_point
    GetNow(){
        using namespace std::chrono;
        return system_clock::now();
    }

    static uint32_t
    GetCurrentTime(){
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

    static inline std::string
    GetCurrentTimeFormatted(){
        time_t current_time;
        time(&current_time);
        struct tm* timeinfo = localtime(&current_time);
        char buff[256];
        strftime(buff, sizeof(buff), "%Y%m%d-%H%M%S", timeinfo);
        return std::string(buff);
    }
}

// Command Line Flags
DECLARE_string(path);
DECLARE_uint32(port);
DECLARE_uint32(heap_size);

//TODO: remove these flags
DECLARE_string(peer_address);
DECLARE_uint32(peer_port);

#define TOKEN_BLOCKCHAIN_HOME (FLAGS_path)

#endif //TOKEN_COMMON_H