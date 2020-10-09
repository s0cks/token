#include "token.h"
#include "heap_dump_writer.h"

namespace Token{
    bool HeapDumpWriter::WriteHeapDump(){
        MemoryRegion* region = Allocator::GetRegion();

        WriteString(Token::GetVersion());
        WriteLong(GetCurrentTimestamp());
        WriteLong(region->GetSize());
        if(!WriteRegion(region)){
            LOG(ERROR) << "couldn't write memory region of size " << region->GetSize() << " to heap dump: " << GetFilename();
            return false;
        }

        LOG(INFO) << "heap dump written: " << GetFilename();
        return true;
    }
}