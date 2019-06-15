#include <random>
#include "uuid.h"

namespace Token{
    UUID& UUID::v4(){
        static std::random_device rd;
        static std::uniform_int_distribution<uint64_t> dist(0, (uint64_t)(~0));
        UUID uuid;
        uuid.ab = dist(rd);
        uuid.cd = dist(rd);
        uuid.ab = (uuid.ab & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
        uuid.cd = (uuid.cd & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
        return uuid;
    }
}