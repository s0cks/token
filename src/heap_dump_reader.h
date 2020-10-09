#ifndef TOKEN_HEAP_DUMP_READER_H
#define TOKEN_HEAP_DUMP_READER_H

#include "heap_dump.h"
#include "file_reader.h"

namespace Token{
    class HeapDumpReader : public BinaryFileReader{
    public:
        HeapDumpReader(const std::string& filename):
            BinaryFileReader(filename){}
        ~HeapDumpReader() = default;

        HeapDump* ReadHeapDump();
    };
}

#endif //TOKEN_HEAP_DUMP_READER_H