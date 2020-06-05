#ifndef TOKEN_LAYOUT_H
#define TOKEN_LAYOUT_H

#include <cstdint>

namespace Token{
    enum{
        kTypeOffset = 0,
        kTypeLength = 1,
        kSizeOffset = kTypeLength,
        kSizeLength = 4,
        kDataOffset = (kTypeLength + kSizeLength),
        kHeaderSize = kDataOffset,
        kByteBufferSize = 6144
    };
}

#endif //TOKEN_LAYOUT_H
