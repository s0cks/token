#include "heap_dump.h"

namespace Token{
    static inline void
    PrintHeapDump(HeapDump* dump){
        LOG(INFO) << "Heap Dump: " << dump->GetFilename();
        LOG(INFO) << "Created: " << GetTimestampFormattedReadable(dump->GetTimestamp());
        LOG(INFO) << "Version: " << dump->GetVersion();
        LOG(INFO) << "Memory Region Size (Bytes): " << dump->GetRegion()->GetSize();
        LOG(INFO) << "Heap Size (Bytes): " << dump->GetOldHeap()->GetTotalSize();
        LOG(INFO) << "Semispace Size (Bytes): " << dump->GetOldHeap()->GetSemispaceSize();
    }

    void HeapDumpInspector::HandleStatusCommand(HeapDumpInspectorCommand* cmd){
        PrintHeapDump(GetHeapDump());
    }
}