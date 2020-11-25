#include "heap.h"
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

    void HeapDumpInspector::HandlePrintObjectsCommand(HeapDumpInspectorCommand* cmd){
        ObjectPointerPrinter printer;
        Space space = cmd->GetNextArgumentSpace();
        switch(space){
            case Space::kOldHeap:{
                LOG(INFO) << "Old Heap:";
                if(!GetNewHeap()->VisitObjects(&printer)){
                    LOG(ERROR) << "cannot print old heap objects.";
                    return;
                }
                return;
            }
            case Space::kNewHeap:{
                LOG(INFO) << "New Heap:";
                if(!GetNewHeap()->VisitObjects(&printer)){
                    LOG(ERROR) << "cannot print new heap objects.";
                    return;
                }
                return;
            }
            case Space::kStackSpace:
                //TODO: implement
                LOG(WARNING) << "not implemented yet";
                return;
        }
    }
}