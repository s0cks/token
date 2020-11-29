#ifndef TOKEN_GCMODE_NONE

#include "heap_dump.h"
#include "heap_dump_reader.h"
#include "heap_dump_writer.h"

namespace Token{
    bool HeapDump::WriteHeapDump(){
        HeapDumpWriter writer;
        return writer.WriteHeapDump();
    }

    HeapDump* HeapDump::ReadHeapDump(const std::string& filename){
        HeapDumpReader reader(filename);
        return reader.ReadHeapDump();
    }
}

#endif//TOKEN_GCMODE_NONE