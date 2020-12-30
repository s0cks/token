#include "test_suite.h"
#include "hash.h"
#include "block.h"

namespace Token{
    static inline void
    GenerateHashList(HashList& hashes, int64_t size){
        for(int64_t idx = 0; idx < size; idx++)
            hashes.insert(Hash::GenerateNonce());
    }

    TEST(TestHashList, test_serialization){
        HashList h1;
        GenerateHashList(h1, Block::kNumberOfGenesisOutputs);

        int64_t size = GetBufferSize(h1);
        uint8_t* bytes = (uint8_t*)malloc(sizeof(uint8_t)*size);
        Encode(h1, bytes, size);

        HashList h2;
        Decode(bytes, size, h2);

        ASSERT_EQ(h1, h2);
    }
}