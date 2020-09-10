#ifndef TOKEN_HEAP_DUMP_WRITER_H
#define TOKEN_HEAP_DUMP_WRITER_H

#include "heap_dump.h"

namespace Token{
    class HeapDumpWriter : public BinaryFileWriter{
    private:
        bool WriteStackSpace();
        bool WriteHeap(Heap* heap);
    public:
        HeapDumpWriter(const std::string& filename):
            BinaryFileWriter(filename){}
        ~HeapDumpWriter() = default;

        bool WriteHeapDump();
    };
}

#endif //TOKEN_HEAP_DUMP_WRITER_H