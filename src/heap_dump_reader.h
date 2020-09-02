#ifndef TOKEN_HEAP_DUMP_READER_H
#define TOKEN_HEAP_DUMP_READER_H

#include "heap_dump.h"

namespace Token{
    class HeapDumpReader : public BinaryFileReader{
    private:
        bool ReadMemoryRegion(MemoryRegion* region, size_t size);
    public:
        HeapDumpReader(const std::string& filename):
            BinaryFileReader(filename){}
        ~HeapDumpReader() = default;

        inline HeapDumpSectionHeader
        ReadSectionHeader(){
            return (HeapDumpSectionHeader)ReadLong();
        }

        HeapDump* ReadHeapDump();
    };
}

#endif //TOKEN_HEAP_DUMP_READER_H