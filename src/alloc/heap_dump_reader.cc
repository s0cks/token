#ifndef TOKEN_GCMODE_NONE

#include "heap_dump_reader.h"

namespace Token{
    HeapDump* HeapDumpReader::ReadHeapDump(){
        std::string version = ReadString();
        Timestamp timestamp = ReadLong();

        intptr_t region_size = ReadLong();
        MemoryRegion* region = new MemoryRegion(region_size);
        if(!ReadRegion(region, region_size)){
            LOG(ERROR) << "couldn't read memory region of size " << region_size << " from heap dump: " << GetFilename();
            delete region;
            return nullptr;
        }

        intptr_t heap_size = region_size / 2; // Externalize?
        intptr_t semispace_size = heap_size / 2; // Externalize?
        return new HeapDump(GetFilename(), version, timestamp, region, heap_size, semispace_size);
    }
}

#endif//TTOKEN_GCMODE_NONE