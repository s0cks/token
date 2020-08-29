#include "heap_dump.h"
#include "bitfield.h"

namespace Token{
    typedef uint64_t kHeapDumpSectionHeader;

    enum HeapDumpSectionHeaderLayout{
        kSpaceFieldPosition = 0,
        kSpaceFieldSize = 16,
        kSizeFieldPosition = kSpaceFieldPosition + kSpaceFieldSize,
        kSizeFieldSize = 32,
    };

    class SpaceField : public BitField<uint64_t, Space, kSpaceFieldPosition, kSpaceFieldSize>{};
    class SizeField : public BitField<uint64_t, uint32_t, kSizeFieldPosition, kSizeFieldSize>{};

    static inline uint64_t
    CreateSectionHeader(Heap* heap){
        uint64_t header = 0;
        header = SpaceField::Update(heap->GetSpace(), header);
        header = SizeField::Update(heap->GetTotalSize(), header);
        return header;
    }

    class HeapDumpWriter{

    };

    class HeapDumpReader{

    };
}