#ifndef TOKEN_ALLOC_H
#define TOKEN_ALLOC_H

#include <cstdlib>
#include <cstdint>
#include <ostream>
#include "common.h"

namespace Token{
    class Allocator{
    public:
        typedef uintptr_t AllocHeader;

        enum class AllocColor{
            kFree = 0,
            kWhite,
            kGray,
            kBlack
        };
    private:
        Allocator();
    public:
        ~Allocator();

        static Allocator* GetInstance();
    };
}

#endif //TOKEN_ALLOC_H