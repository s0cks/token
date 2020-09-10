#ifndef TOKEN_HEAP_DUMP_WRITER_H
#define TOKEN_HEAP_DUMP_WRITER_H

#include "heap_dump.h"

namespace Token{
    class HeapDumpWriter : public BinaryFileWriter{
    private:
        Heap* eden_;
        Heap* survivor_;

        bool WriteStackSpace();
        bool WriteHeap(Heap* heap);
    public:
        HeapDumpWriter(const std::string& filename):
            BinaryFileWriter(filename),
            eden_(Allocator::GetNewSpace()),
            survivor_(Allocator::GetOldSpace()){}
        ~HeapDumpWriter() = default;

        Heap* GetEdenHeap() const{
            return eden_;
        }

        Heap* GetSurvivorHeap() const{
            return survivor_;
        }

        bool WriteHeapDump();
    };
}

#endif //TOKEN_HEAP_DUMP_WRITER_H